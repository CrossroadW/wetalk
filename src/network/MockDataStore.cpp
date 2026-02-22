#include "MockDataStore.h"

#include <algorithm>
#include <chrono>

namespace wechat::network {

MockDataStore::MockDataStore()
    : idCounter(0),
      dbm_(":memory:"),
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

int64_t MockDataStore::nextId() {
    return ++idCounter;
}

// ── 用户 / 认证 ──

std::string MockDataStore::addUser(std::string const &username,
                                   std::string const &password) {
    auto id = std::to_string(nextId());
    core::User user{id};
    userDao_.insert(user);
    passwords_[username] = password;
    userIdToName_[id] = username;
    return id;
}

std::string MockDataStore::authenticate(std::string const &username,
                                        std::string const &password) {
    auto it = passwords_.find(username);
    if (it == passwords_.end() || it->second != password) {
        return {};
    }
    // 找到 username 对应的 userId
    for (auto &[uid, name] : userIdToName_) {
        if (name == username) return uid;
    }
    return {};
}

std::string MockDataStore::createToken(std::string const &userId) {
    auto token = "tok_" + std::to_string(nextId());
    tokens_[token] = userId;
    return token;
}

std::string MockDataStore::resolveToken(std::string const &token) {
    auto it = tokens_.find(token);
    return it != tokens_.end() ? it->second : std::string{};
}

void MockDataStore::removeToken(std::string const &token) {
    tokens_.erase(token);
}

core::User *MockDataStore::findUser(std::string const &userId) {
    auto opt = userDao_.findById(userId);
    if (!opt) return nullptr;
    userCache_[userId] = *opt;
    return &userCache_[userId];
}

std::vector<core::User> MockDataStore::searchUsers(std::string const &keyword) {
    auto all = userDao_.findAll();
    std::vector<core::User> result;
    for (auto &u : all) {
        // 搜索 userId 或 username
        auto nameIt = userIdToName_.find(u.id);
        std::string name = nameIt != userIdToName_.end() ? nameIt->second : "";
        if (name.find(keyword) != std::string::npos ||
            u.id.find(keyword) != std::string::npos) {
            result.push_back(u);
        }
    }
    return result;
}

// ── 好友 ──

void MockDataStore::addFriendship(std::string const &a, std::string const &b) {
    friendshipDao_.add(a, b);
}

void MockDataStore::removeFriendship(std::string const &a,
                                     std::string const &b) {
    friendshipDao_.remove(a, b);
}

bool MockDataStore::areFriends(std::string const &a, std::string const &b) {
    return friendshipDao_.isFriend(a, b);
}

std::vector<std::string>
MockDataStore::getFriendIds(std::string const &userId) {
    return friendshipDao_.findFriends(userId);
}

// ── 群组 ──

core::Group &
MockDataStore::createGroup(std::string const &ownerId,
                           std::vector<std::string> const &memberIds) {
    auto id = std::to_string(nextId());
    core::Group group{id, ownerId, memberIds};
    groupDao_.insertGroup(group, now());
    // 缓存以保证返回引用稳定
    groupCache_[id] = group;
    return groupCache_[id];
}

core::Group *MockDataStore::findGroup(std::string const &groupId) {
    auto opt = groupDao_.findGroupById(groupId);
    if (!opt) return nullptr;
    // 加载成员列表
    opt->memberIds = groupDao_.findMemberIds(groupId);
    groupCache_[groupId] = *opt;
    return &groupCache_[groupId];
}

void MockDataStore::addGroupMember(std::string const &groupId,
                                   std::string const &userId) {
    groupDao_.addMember(groupId, userId, now());
}

void MockDataStore::removeGroupMember(std::string const &groupId,
                                      std::string const &userId) {
    groupDao_.removeMember(groupId, userId, now());
}

void MockDataStore::removeGroup(std::string const &groupId) {
    groupDao_.removeGroup(groupId);
    groupCache_.erase(groupId);
}

std::vector<core::Group>
MockDataStore::getGroupsByUser(std::string const &userId) {
    auto groupIds = groupDao_.findGroupIdsByUser(userId);
    std::vector<core::Group> result;
    for (auto &gid : groupIds) {
        auto opt = groupDao_.findGroupById(gid);
        if (opt) {
            opt->memberIds = groupDao_.findMemberIds(gid);
            result.push_back(*opt);
        }
    }
    return result;
}

// ── 消息 ──

core::Message &MockDataStore::addMessage(std::string const &senderId,
                                         std::string const &chatId,
                                         int64_t replyTo,
                                         core::MessageContent const &content) {
    auto id = nextId();
    auto ts = now();
    core::Message msg{id, senderId, chatId, replyTo, content,
                      ts, 0,        false,  0,       0};
    messageDao_.insert(msg);
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

std::vector<core::Message> MockDataStore::getMessagesAfter(std::string const &chatId,
                                                           int64_t afterId,
                                                           int limit) {
    return messageDao_.findAfter(chatId, afterId, limit);
}

std::vector<core::Message> MockDataStore::getMessagesBefore(std::string const &chatId,
                                                            int64_t beforeId,
                                                            int limit) {
    return messageDao_.findBefore(chatId, beforeId, limit);
}

std::vector<core::Message> MockDataStore::getMessagesUpdatedAfter(
    std::string const &chatId, int64_t startId, int64_t endId,
    int64_t updatedAt, int limit) {
    return messageDao_.findUpdatedAfter(chatId, startId, endId, updatedAt, limit);
}

// ── 朋友圈 ──

Moment &MockDataStore::addMoment(std::string const &authorId,
                                 std::string const &text,
                                 std::vector<std::string> const &imageIds) {
    auto id = nextId();
    auto ts = now();
    Moment moment{id, authorId, text, imageIds, ts, {}, {}};
    auto [it, _] = moments_.emplace(id, std::move(moment));
    momentTimeline_.insert(momentTimeline_.begin(), id);
    return it->second;
}

Moment *MockDataStore::findMoment(int64_t momentId) {
    auto it = moments_.find(momentId);
    return it != moments_.end() ? &it->second : nullptr;
}

std::vector<Moment>
MockDataStore::getMoments(std::set<std::string> const &visibleUserIds,
                          int64_t beforeTs, int limit) {
    std::vector<Moment> result;
    for (auto &id: momentTimeline_) {
        auto it = moments_.find(id);
        if (it == moments_.end()) {
            continue;
        }
        auto &m = it->second;
        if (m.timestamp >= beforeTs) {
            continue;
        }
        if (!visibleUserIds.contains(m.authorId)) {
            continue;
        }
        result.push_back(m);
        if (static_cast<int>(result.size()) >= limit) {
            break;
        }
    }
    return result;
}

} // namespace wechat::network
