#include "MockDataStore.h"

#include <algorithm>
#include <chrono>

namespace wechat::network {

MockDataStore::MockDataStore()
    : dbm_(":memory:"),
      userDao_(dbm_.db()),
      friendshipDao_(dbm_.db()),
      groupDao_(dbm_.db()),
      messageDao_(dbm_.db()) {
    dbm_.initSchema();
}

int64_t MockDataStore::now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ── 用户 / 认证 ──

int64_t MockDataStore::addUser(std::string const &username,
                               std::string const &password) {
    core::User user{};
    auto id = userDao_.insert(user);
    passwords_[username] = password;
    userIdToName_[id] = username;
    return id;
}

int64_t MockDataStore::authenticate(std::string const &username,
                                    std::string const &password) {
    auto it = passwords_.find(username);
    if (it == passwords_.end() || it->second != password) {
        return 0;
    }
    for (auto &[uid, name] : userIdToName_) {
        if (name == username) return uid;
    }
    return 0;
}

std::string MockDataStore::createToken(int64_t userId) {
    static int64_t tokenCounter = 0;
    auto token = "tok_" + std::to_string(++tokenCounter);
    tokens_[token] = userId;
    return token;
}

int64_t MockDataStore::resolveToken(std::string const &token) {
    auto it = tokens_.find(token);
    return it != tokens_.end() ? it->second : 0;
}

void MockDataStore::removeToken(std::string const &token) {
    tokens_.erase(token);
}

core::User *MockDataStore::findUser(int64_t userId) {
    auto opt = userDao_.findById(userId);
    if (!opt) return nullptr;
    userCache_[userId] = *opt;
    return &userCache_[userId];
}

std::vector<core::User> MockDataStore::searchUsers(std::string const &keyword) {
    auto all = userDao_.findAll();
    std::vector<core::User> result;
    for (auto &u : all) {
        auto nameIt = userIdToName_.find(u.id);
        std::string name = nameIt != userIdToName_.end() ? nameIt->second : "";
        auto idStr = std::to_string(u.id);
        if (name.find(keyword) != std::string::npos ||
            idStr.find(keyword) != std::string::npos) {
            result.push_back(u);
        }
    }
    return result;
}

// ── 好友 ──

void MockDataStore::addFriendship(int64_t a, int64_t b) {
    friendshipDao_.add(a, b);
}

void MockDataStore::removeFriendship(int64_t a, int64_t b) {
    friendshipDao_.remove(a, b);
}

bool MockDataStore::areFriends(int64_t a, int64_t b) {
    return friendshipDao_.isFriend(a, b);
}

std::vector<int64_t>
MockDataStore::getFriendIds(int64_t userId) {
    return friendshipDao_.findFriends(userId);
}

// ── 群组 ──

core::Group &
MockDataStore::createGroup(int64_t ownerId,
                           std::vector<int64_t> const &memberIds) {
    core::Group group{0, ownerId, memberIds};
    auto id = groupDao_.insertGroup(group, now());
    group.id = id;
    groupCache_[id] = group;
    return groupCache_[id];
}

core::Group *MockDataStore::findGroup(int64_t groupId) {
    auto opt = groupDao_.findGroupById(groupId);
    if (!opt) return nullptr;
    opt->memberIds = groupDao_.findMemberIds(groupId);
    groupCache_[groupId] = *opt;
    return &groupCache_[groupId];
}

void MockDataStore::addGroupMember(int64_t groupId, int64_t userId) {
    groupDao_.addMember(groupId, userId, now());
}

void MockDataStore::removeGroupMember(int64_t groupId, int64_t userId) {
    groupDao_.removeMember(groupId, userId, now());
}

void MockDataStore::removeGroup(int64_t groupId) {
    groupDao_.removeGroup(groupId);
    groupCache_.erase(groupId);
}

std::vector<core::Group>
MockDataStore::getGroupsByUser(int64_t userId) {
    auto groupIds = groupDao_.findGroupIdsByUser(userId);
    std::vector<core::Group> result;
    for (auto gid : groupIds) {
        auto opt = groupDao_.findGroupById(gid);
        if (opt) {
            opt->memberIds = groupDao_.findMemberIds(gid);
            result.push_back(*opt);
        }
    }
    return result;
}

// ── 消息 ──

core::Message &MockDataStore::addMessage(int64_t senderId,
                                         int64_t chatId,
                                         int64_t replyTo,
                                         core::MessageContent const &content) {
    auto ts = now();
    core::Message msg{0, senderId, chatId, replyTo, content,
                      ts, 0,        false,  0,       0};
    auto id = messageDao_.insert(msg);
    msg.id = id;
    messageCache_[id] = msg;
    return messageCache_[id];
}

core::Message *MockDataStore::findMessage(int64_t messageId) {
    auto opt = messageDao_.findById(messageId);
    if (!opt) return nullptr;
    messageCache_[messageId] = *opt;
    return &messageCache_[messageId];
}

void MockDataStore::saveMessage(const core::Message& msg) {
    messageDao_.update(msg);
}

std::vector<core::Message> MockDataStore::getMessagesAfter(int64_t chatId,
                                                           int64_t afterId,
                                                           int limit) {
    return messageDao_.findAfter(chatId, afterId, limit);
}

std::vector<core::Message> MockDataStore::getMessagesBefore(int64_t chatId,
                                                            int64_t beforeId,
                                                            int limit) {
    return messageDao_.findBefore(chatId, beforeId, limit);
}

std::vector<core::Message> MockDataStore::getMessagesUpdatedAfter(
    int64_t chatId, int64_t startId, int64_t endId,
    int64_t updatedAt, int limit) {
    return messageDao_.findUpdatedAfter(chatId, startId, endId, updatedAt, limit);
}

// ── 朋友圈 ──

Moment &MockDataStore::addMoment(int64_t authorId,
                                 std::string const &text,
                                 std::vector<std::string> const &imageIds) {
    auto ts = now();
    // 插入 moments 表
    SQLite::Statement stmt(dbm_.db(),
        "INSERT INTO moments (author_id, text, timestamp) VALUES (?, ?, ?)");
    stmt.bind(1, authorId);
    stmt.bind(2, text);
    stmt.bind(3, ts);
    stmt.exec();
    auto id = dbm_.db().getLastInsertRowid();

    // 插入 moment_images
    for (int i = 0; i < static_cast<int>(imageIds.size()); ++i) {
        SQLite::Statement imgStmt(dbm_.db(),
            "INSERT INTO moment_images (moment_id, image_id, sort_order) VALUES (?, ?, ?)");
        imgStmt.bind(1, id);
        imgStmt.bind(2, imageIds[i]);
        imgStmt.bind(3, i);
        imgStmt.exec();
    }

    momentCache_[id] = Moment{id, authorId, text, imageIds, ts, {}, {}};
    return momentCache_[id];
}

Moment MockDataStore::loadMoment(int64_t momentId) {
    Moment m;
    m.id = momentId;

    // 基本信息
    SQLite::Statement stmt(dbm_.db(),
        "SELECT author_id, text, timestamp FROM moments WHERE id = ?");
    stmt.bind(1, momentId);
    if (!stmt.executeStep()) return m;
    m.authorId = stmt.getColumn(0).getInt64();
    m.text = stmt.getColumn(1).getString();
    m.timestamp = stmt.getColumn(2).getInt64();

    // images
    SQLite::Statement imgStmt(dbm_.db(),
        "SELECT image_id FROM moment_images WHERE moment_id = ? ORDER BY sort_order");
    imgStmt.bind(1, momentId);
    while (imgStmt.executeStep()) {
        m.imageIds.push_back(imgStmt.getColumn(0).getString());
    }

    // likes
    SQLite::Statement likeStmt(dbm_.db(),
        "SELECT user_id FROM moment_likes WHERE moment_id = ?");
    likeStmt.bind(1, momentId);
    while (likeStmt.executeStep()) {
        m.likedBy.push_back(likeStmt.getColumn(0).getInt64());
    }

    // comments
    SQLite::Statement cmtStmt(dbm_.db(),
        "SELECT id, author_id, text, timestamp FROM moment_comments WHERE moment_id = ? ORDER BY id");
    cmtStmt.bind(1, momentId);
    while (cmtStmt.executeStep()) {
        m.comments.push_back({
            cmtStmt.getColumn(0).getInt64(),
            cmtStmt.getColumn(1).getInt64(),
            cmtStmt.getColumn(2).getString(),
            cmtStmt.getColumn(3).getInt64()
        });
    }

    return m;
}

Moment *MockDataStore::findMoment(int64_t momentId) {
    // 从 SQLite 加载到缓存
    SQLite::Statement stmt(dbm_.db(),
        "SELECT id FROM moments WHERE id = ?");
    stmt.bind(1, momentId);
    if (!stmt.executeStep()) return nullptr;
    momentCache_[momentId] = loadMoment(momentId);
    return &momentCache_[momentId];
}

bool MockDataStore::hasLiked(int64_t momentId, int64_t userId) {
    SQLite::Statement stmt(dbm_.db(),
        "SELECT 1 FROM moment_likes WHERE moment_id = ? AND user_id = ?");
    stmt.bind(1, momentId);
    stmt.bind(2, userId);
    return stmt.executeStep();
}

void MockDataStore::addLike(int64_t momentId, int64_t userId) {
    SQLite::Statement stmt(dbm_.db(),
        "INSERT OR IGNORE INTO moment_likes (moment_id, user_id) VALUES (?, ?)");
    stmt.bind(1, momentId);
    stmt.bind(2, userId);
    stmt.exec();
}

int64_t MockDataStore::addComment(int64_t momentId, int64_t authorId,
                                  const std::string& text) {
    auto ts = now();
    SQLite::Statement stmt(dbm_.db(),
        "INSERT INTO moment_comments (moment_id, author_id, text, timestamp) VALUES (?, ?, ?, ?)");
    stmt.bind(1, momentId);
    stmt.bind(2, authorId);
    stmt.bind(3, text);
    stmt.bind(4, ts);
    stmt.exec();
    return dbm_.db().getLastInsertRowid();
}

std::vector<Moment>
MockDataStore::getMoments(std::set<int64_t> const &visibleUserIds,
                          int64_t beforeTs, int limit) {
    // 查询可见用户的动态，按时间倒序
    std::vector<Moment> result;
    SQLite::Statement stmt(dbm_.db(),
        "SELECT id FROM moments WHERE timestamp < ? ORDER BY timestamp DESC");
    stmt.bind(1, beforeTs);
    while (stmt.executeStep()) {
        auto id = stmt.getColumn(0).getInt64();
        auto m = loadMoment(id);
        if (!visibleUserIds.contains(m.authorId)) continue;
        result.push_back(std::move(m));
        if (static_cast<int>(result.size()) >= limit) break;
    }
    return result;
}

} // namespace wechat::network
