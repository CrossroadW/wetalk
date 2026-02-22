#include "MockDataStore.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>

using json = nlohmann::json;

namespace {

// ── JSON 序列化（从 MessageDao 搬入）──

json metaToJson(const wechat::core::ResourceMeta& m) {
    return {{"size", m.size}, {"filename", m.filename}, {"extra", m.extra}};
}

wechat::core::ResourceMeta jsonToMeta(const json& j) {
    wechat::core::ResourceMeta m;
    m.size = j.value("size", std::size_t{0});
    m.filename = j.value("filename", "");
    if (j.contains("extra")) {
        m.extra = j["extra"].get<std::map<std::string, std::string>>();
    }
    return m;
}

json blockToJson(const wechat::core::ContentBlock& block) {
    return std::visit([](auto&& arg) -> json {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return {{"type", 0}};
        } else if constexpr (std::is_same_v<T, wechat::core::TextContent>) {
            return {{"type", 1}, {"text", arg.text}};
        } else if constexpr (std::is_same_v<T, wechat::core::ResourceContent>) {
            return {{"type", 2},
                    {"resourceId", arg.resourceId},
                    {"resType", static_cast<int>(arg.type)},
                    {"resSubtype", static_cast<int>(arg.subtype)},
                    {"meta", metaToJson(arg.meta)}};
        }
    }, block);
}

wechat::core::ContentBlock jsonToBlock(const json& j) {
    int type = j.value("type", 0);
    switch (type) {
    case 1:
        return wechat::core::TextContent{j.value("text", "")};
    case 2: {
        wechat::core::ResourceContent rc;
        rc.resourceId = j.value("resourceId", "");
        rc.type = static_cast<wechat::core::ResourceType>(j.value("resType", 0));
        rc.subtype = static_cast<wechat::core::ResourceSubtype>(j.value("resSubtype", 0));
        if (j.contains("meta")) rc.meta = jsonToMeta(j["meta"]);
        return rc;
    }
    default:
        return std::monostate{};
    }
}

std::string serializeContent(const wechat::core::MessageContent& content) {
    json arr = json::array();
    for (const auto& block : content) {
        arr.push_back(blockToJson(block));
    }
    return arr.dump();
}

wechat::core::MessageContent deserializeContent(const std::string& str) {
    wechat::core::MessageContent content;
    auto arr = json::parse(str, nullptr, false);
    if (arr.is_discarded() || !arr.is_array()) return content;
    for (const auto& item : arr) {
        content.push_back(jsonToBlock(item));
    }
    return content;
}

} // anonymous namespace

namespace wechat::network {

// ── 构造 / Schema ──

MockDataStore::MockDataStore()
    : db_(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    db_.exec("PRAGMA journal_mode=WAL");
    db_.exec("PRAGMA foreign_keys=ON");
    initSchema();
}

void MockDataStore::initSchema() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS tokens (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS groups_ (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            owner_id INTEGER,
            updated_at INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS group_members (
            group_id INTEGER,
            user_id INTEGER,
            joined_at INTEGER NOT NULL,
            removed INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0,
            PRIMARY KEY (group_id, user_id)
        );

        CREATE TABLE IF NOT EXISTS friendships (
            user_id_a INTEGER,
            user_id_b INTEGER,
            PRIMARY KEY (user_id_a, user_id_b)
        );

        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER,
            chat_id INTEGER NOT NULL,
            reply_to INTEGER DEFAULT 0,
            content_data TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            edited_at INTEGER DEFAULT 0,
            revoked INTEGER DEFAULT 0,
            read_count INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0
        );

        CREATE INDEX IF NOT EXISTS idx_group_members_user
            ON group_members(user_id);
        CREATE INDEX IF NOT EXISTS idx_messages_chat
            ON messages(chat_id, id);
        CREATE INDEX IF NOT EXISTS idx_messages_reply
            ON messages(reply_to);
        CREATE INDEX IF NOT EXISTS idx_messages_updated
            ON messages(chat_id, updated_at);

        CREATE TABLE IF NOT EXISTS moments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            author_id INTEGER NOT NULL,
            text TEXT NOT NULL DEFAULT '',
            timestamp INTEGER NOT NULL,
            updated_at INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS moment_images (
            moment_id INTEGER NOT NULL,
            image_id TEXT NOT NULL,
            sort_order INTEGER DEFAULT 0,
            PRIMARY KEY (moment_id, image_id)
        );

        CREATE TABLE IF NOT EXISTS moment_likes (
            moment_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY (moment_id, user_id)
        );

        CREATE TABLE IF NOT EXISTS moment_comments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            moment_id INTEGER NOT NULL,
            author_id INTEGER NOT NULL,
            text TEXT NOT NULL,
            timestamp INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_moments_author
            ON moments(author_id);
        CREATE INDEX IF NOT EXISTS idx_moment_comments_moment
            ON moment_comments(moment_id);
    )");
}

int64_t MockDataStore::now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::pair<int64_t, int64_t> MockDataStore::orderedPair(int64_t a, int64_t b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

// ── 用户 / 认证 ──

int64_t MockDataStore::addUser(std::string const &username,
                               std::string const &password) {
    SQLite::Statement stmt(db_,
        "INSERT INTO users (username, password) VALUES (?, ?)");
    stmt.bind(1, username);
    stmt.bind(2, password);
    stmt.exec();
    return db_.getLastInsertRowid();
}

int64_t MockDataStore::authenticate(std::string const &username,
                                    std::string const &password) {
    SQLite::Statement stmt(db_,
        "SELECT id FROM users WHERE username = ? AND password = ?");
    stmt.bind(1, username);
    stmt.bind(2, password);
    if (!stmt.executeStep()) return 0;
    return stmt.getColumn(0).getInt64();
}

std::string MockDataStore::createToken(int64_t userId) {
    static int64_t tokenCounter = 0;
    auto token = "tok_" + std::to_string(++tokenCounter);
    SQLite::Statement stmt(db_,
        "INSERT INTO tokens (token, user_id) VALUES (?, ?)");
    stmt.bind(1, token);
    stmt.bind(2, userId);
    stmt.exec();
    return token;
}

int64_t MockDataStore::resolveToken(std::string const &token) {
    SQLite::Statement stmt(db_,
        "SELECT user_id FROM tokens WHERE token = ?");
    stmt.bind(1, token);
    if (!stmt.executeStep()) return 0;
    return stmt.getColumn(0).getInt64();
}

void MockDataStore::removeToken(std::string const &token) {
    SQLite::Statement stmt(db_, "DELETE FROM tokens WHERE token = ?");
    stmt.bind(1, token);
    stmt.exec();
}

std::optional<core::User> MockDataStore::findUser(int64_t userId) {
    SQLite::Statement stmt(db_, "SELECT id FROM users WHERE id = ?");
    stmt.bind(1, userId);
    if (!stmt.executeStep()) return std::nullopt;
    return core::User{stmt.getColumn(0).getInt64()};
}

std::vector<core::User> MockDataStore::searchUsers(std::string const &keyword) {
    SQLite::Statement stmt(db_,
        "SELECT id, username FROM users WHERE username LIKE ? OR CAST(id AS TEXT) LIKE ?");
    auto pattern = "%" + keyword + "%";
    stmt.bind(1, pattern);
    stmt.bind(2, pattern);
    std::vector<core::User> result;
    while (stmt.executeStep()) {
        result.push_back(core::User{stmt.getColumn(0).getInt64()});
    }
    return result;
}

// ── 好友 ──

void MockDataStore::addFriendship(int64_t a, int64_t b) {
    auto [lo, hi] = orderedPair(a, b);
    SQLite::Statement stmt(db_,
        "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)");
    stmt.bind(1, lo);
    stmt.bind(2, hi);
    stmt.exec();
}

void MockDataStore::removeFriendship(int64_t a, int64_t b) {
    auto [lo, hi] = orderedPair(a, b);
    SQLite::Statement stmt(db_,
        "DELETE FROM friendships WHERE user_id_a = ? AND user_id_b = ?");
    stmt.bind(1, lo);
    stmt.bind(2, hi);
    stmt.exec();
}

bool MockDataStore::areFriends(int64_t a, int64_t b) {
    auto [lo, hi] = orderedPair(a, b);
    SQLite::Statement stmt(db_,
        "SELECT 1 FROM friendships WHERE user_id_a = ? AND user_id_b = ?");
    stmt.bind(1, lo);
    stmt.bind(2, hi);
    return stmt.executeStep();
}

std::vector<int64_t> MockDataStore::getFriendIds(int64_t userId) {
    std::vector<int64_t> friends;
    SQLite::Statement s1(db_,
        "SELECT user_id_b FROM friendships WHERE user_id_a = ?");
    s1.bind(1, userId);
    while (s1.executeStep()) {
        friends.push_back(s1.getColumn(0).getInt64());
    }
    SQLite::Statement s2(db_,
        "SELECT user_id_a FROM friendships WHERE user_id_b = ?");
    s2.bind(1, userId);
    while (s2.executeStep()) {
        friends.push_back(s2.getColumn(0).getInt64());
    }
    return friends;
}

// ── 群组 ──

std::vector<int64_t> MockDataStore::findGroupMemberIds(int64_t groupId) {
    std::vector<int64_t> ids;
    SQLite::Statement stmt(db_,
        "SELECT user_id FROM group_members WHERE group_id = ? AND removed = 0");
    stmt.bind(1, groupId);
    while (stmt.executeStep()) {
        ids.push_back(stmt.getColumn(0).getInt64());
    }
    return ids;
}

std::vector<int64_t> MockDataStore::findGroupIdsByUser(int64_t userId) {
    std::vector<int64_t> ids;
    SQLite::Statement stmt(db_,
        "SELECT group_id FROM group_members WHERE user_id = ? AND removed = 0");
    stmt.bind(1, userId);
    while (stmt.executeStep()) {
        ids.push_back(stmt.getColumn(0).getInt64());
    }
    return ids;
}

core::Group
MockDataStore::createGroup(int64_t ownerId,
                           std::vector<int64_t> const &memberIds) {
    auto ts = now();
    SQLite::Statement stmt(db_,
        "INSERT INTO groups_ (owner_id, updated_at) VALUES (?, ?)");
    stmt.bind(1, ownerId);
    stmt.bind(2, ts);
    stmt.exec();
    auto id = db_.getLastInsertRowid();

    for (auto uid : memberIds) {
        SQLite::Statement ms(db_, R"(
            INSERT INTO group_members (group_id, user_id, joined_at, removed, updated_at)
            VALUES (?, ?, ?, 0, ?)
            ON CONFLICT(group_id, user_id) DO UPDATE
                SET removed = 0, updated_at = excluded.updated_at
        )");
        ms.bind(1, id);
        ms.bind(2, uid);
        ms.bind(3, ts);
        ms.bind(4, ts);
        ms.exec();
    }

    return core::Group{id, ownerId, memberIds};
}

std::optional<core::Group> MockDataStore::findGroup(int64_t groupId) {
    SQLite::Statement stmt(db_,
        "SELECT id, owner_id FROM groups_ WHERE id = ?");
    stmt.bind(1, groupId);
    if (!stmt.executeStep()) return std::nullopt;

    core::Group g;
    g.id = stmt.getColumn(0).getInt64();
    g.ownerId = stmt.getColumn(1).getInt64();
    g.memberIds = findGroupMemberIds(groupId);
    return g;
}

void MockDataStore::addGroupMember(int64_t groupId, int64_t userId) {
    auto ts = now();
    SQLite::Statement stmt(db_, R"(
        INSERT INTO group_members (group_id, user_id, joined_at, removed, updated_at)
        VALUES (?, ?, ?, 0, ?)
        ON CONFLICT(group_id, user_id) DO UPDATE
            SET removed = 0, updated_at = excluded.updated_at
    )");
    stmt.bind(1, groupId);
    stmt.bind(2, userId);
    stmt.bind(3, ts);
    stmt.bind(4, ts);
    stmt.exec();
}

void MockDataStore::removeGroupMember(int64_t groupId, int64_t userId) {
    auto ts = now();
    SQLite::Statement stmt(db_, R"(
        UPDATE group_members SET removed = 1, updated_at = ?
        WHERE group_id = ? AND user_id = ?
    )");
    stmt.bind(1, ts);
    stmt.bind(2, groupId);
    stmt.bind(3, userId);
    stmt.exec();
}

void MockDataStore::removeGroup(int64_t groupId) {
    SQLite::Statement stmt1(db_, "DELETE FROM group_members WHERE group_id = ?");
    stmt1.bind(1, groupId);
    stmt1.exec();
    SQLite::Statement stmt2(db_, "DELETE FROM groups_ WHERE id = ?");
    stmt2.bind(1, groupId);
    stmt2.exec();
}

std::vector<core::Group>
MockDataStore::getGroupsByUser(int64_t userId) {
    auto groupIds = findGroupIdsByUser(userId);
    std::vector<core::Group> result;
    for (auto gid : groupIds) {
        SQLite::Statement stmt(db_,
            "SELECT id, owner_id FROM groups_ WHERE id = ?");
        stmt.bind(1, gid);
        if (stmt.executeStep()) {
            core::Group g;
            g.id = stmt.getColumn(0).getInt64();
            g.ownerId = stmt.getColumn(1).getInt64();
            g.memberIds = findGroupMemberIds(gid);
            result.push_back(g);
        }
    }
    return result;
}

// ── 消息 ──

core::Message MockDataStore::rowToMessage(SQLite::Statement& stmt) {
    core::Message msg;
    msg.id        = stmt.getColumn(0).getInt64();
    msg.senderId  = stmt.getColumn(1).getInt64();
    msg.chatId    = stmt.getColumn(2).getInt64();
    msg.replyTo   = stmt.getColumn(3).getInt64();
    msg.content   = deserializeContent(stmt.getColumn(4).getString());
    msg.timestamp = stmt.getColumn(5).getInt64();
    msg.editedAt  = stmt.getColumn(6).getInt64();
    msg.revoked   = stmt.getColumn(7).getInt() != 0;
    msg.readCount = static_cast<uint32_t>(stmt.getColumn(8).getInt());
    msg.updatedAt = stmt.getColumn(9).getInt64();
    return msg;
}

core::Message MockDataStore::addMessage(int64_t senderId,
                                         int64_t chatId,
                                         int64_t replyTo,
                                         core::MessageContent const &content) {
    auto ts = now();
    SQLite::Statement stmt(db_, R"(
        INSERT INTO messages
        (sender_id, chat_id, reply_to, content_data, timestamp,
         edited_at, revoked, read_count, updated_at)
        VALUES (?, ?, ?, ?, ?, 0, 0, 0, 0)
    )");
    stmt.bind(1, senderId);
    stmt.bind(2, chatId);
    stmt.bind(3, replyTo);
    stmt.bind(4, serializeContent(content));
    stmt.bind(5, ts);
    stmt.exec();
    auto id = db_.getLastInsertRowid();

    return core::Message{id, senderId, chatId, replyTo, content,
                         ts, 0,        false,  0,       0};
}

std::optional<core::Message> MockDataStore::findMessage(int64_t messageId) {
    SQLite::Statement stmt(db_, R"(
        SELECT id, sender_id, chat_id, reply_to, content_data,
               timestamp, edited_at, revoked, read_count, updated_at
        FROM messages WHERE id = ?
    )");
    stmt.bind(1, messageId);
    if (!stmt.executeStep()) return std::nullopt;
    return rowToMessage(stmt);
}

void MockDataStore::saveMessage(const core::Message& msg) {
    SQLite::Statement stmt(db_, R"(
        UPDATE messages SET
            sender_id = ?, chat_id = ?, reply_to = ?, content_data = ?,
            timestamp = ?, edited_at = ?, revoked = ?, read_count = ?, updated_at = ?
        WHERE id = ?
    )");
    stmt.bind(1, msg.senderId);
    stmt.bind(2, msg.chatId);
    stmt.bind(3, msg.replyTo);
    stmt.bind(4, serializeContent(msg.content));
    stmt.bind(5, msg.timestamp);
    stmt.bind(6, msg.editedAt);
    stmt.bind(7, msg.revoked ? 1 : 0);
    stmt.bind(8, static_cast<int>(msg.readCount));
    stmt.bind(9, msg.updatedAt);
    stmt.bind(10, msg.id);
    stmt.exec();
}

std::vector<core::Message> MockDataStore::getMessagesAfter(int64_t chatId,
                                                           int64_t afterId,
                                                           int limit) {
    std::vector<core::Message> result;
    if (afterId == 0) {
        SQLite::Statement stmt(db_, R"(
            SELECT * FROM (
                SELECT id, sender_id, chat_id, reply_to, content_data,
                       timestamp, edited_at, revoked, read_count, updated_at
                FROM messages
                WHERE chat_id = ?
                ORDER BY id DESC LIMIT ?
            ) sub ORDER BY id ASC
        )");
        stmt.bind(1, chatId);
        stmt.bind(2, limit);
        while (stmt.executeStep()) {
            result.push_back(rowToMessage(stmt));
        }
    } else {
        SQLite::Statement stmt(db_, R"(
            SELECT id, sender_id, chat_id, reply_to, content_data,
                   timestamp, edited_at, revoked, read_count, updated_at
            FROM messages
            WHERE chat_id = ? AND id > ?
            ORDER BY id ASC LIMIT ?
        )");
        stmt.bind(1, chatId);
        stmt.bind(2, afterId);
        stmt.bind(3, limit);
        while (stmt.executeStep()) {
            result.push_back(rowToMessage(stmt));
        }
    }
    return result;
}

std::vector<core::Message> MockDataStore::getMessagesBefore(int64_t chatId,
                                                            int64_t beforeId,
                                                            int limit) {
    std::vector<core::Message> result;
    if (beforeId == 0) {
        SQLite::Statement stmt(db_, R"(
            SELECT id, sender_id, chat_id, reply_to, content_data,
                   timestamp, edited_at, revoked, read_count, updated_at
            FROM messages
            WHERE chat_id = ?
            ORDER BY id ASC LIMIT ?
        )");
        stmt.bind(1, chatId);
        stmt.bind(2, limit);
        while (stmt.executeStep()) {
            result.push_back(rowToMessage(stmt));
        }
    } else {
        SQLite::Statement stmt(db_, R"(
            SELECT * FROM (
                SELECT id, sender_id, chat_id, reply_to, content_data,
                       timestamp, edited_at, revoked, read_count, updated_at
                FROM messages
                WHERE chat_id = ? AND id < ?
                ORDER BY id DESC LIMIT ?
            ) sub ORDER BY id ASC
        )");
        stmt.bind(1, chatId);
        stmt.bind(2, beforeId);
        stmt.bind(3, limit);
        while (stmt.executeStep()) {
            result.push_back(rowToMessage(stmt));
        }
    }
    return result;
}

std::vector<core::Message> MockDataStore::getMessagesUpdatedAfter(
    int64_t chatId, int64_t startId, int64_t endId,
    int64_t updatedAt, int limit) {
    std::vector<core::Message> result;
    SQLite::Statement stmt(db_, R"(
        SELECT id, sender_id, chat_id, reply_to, content_data,
               timestamp, edited_at, revoked, read_count, updated_at
        FROM messages
        WHERE chat_id = ? AND id >= ? AND id <= ? AND updated_at > ?
        ORDER BY id ASC LIMIT ?
    )");
    stmt.bind(1, chatId);
    stmt.bind(2, startId);
    stmt.bind(3, endId);
    stmt.bind(4, updatedAt);
    stmt.bind(5, limit);
    while (stmt.executeStep()) {
        result.push_back(rowToMessage(stmt));
    }
    return result;
}

// ── 朋友圈 ──

Moment MockDataStore::addMoment(int64_t authorId,
                                 std::string const &text,
                                 std::vector<std::string> const &imageIds) {
    auto ts = now();
    SQLite::Statement stmt(db_,
        "INSERT INTO moments (author_id, text, timestamp) VALUES (?, ?, ?)");
    stmt.bind(1, authorId);
    stmt.bind(2, text);
    stmt.bind(3, ts);
    stmt.exec();
    auto id = db_.getLastInsertRowid();

    for (int i = 0; i < static_cast<int>(imageIds.size()); ++i) {
        SQLite::Statement imgStmt(db_,
            "INSERT INTO moment_images (moment_id, image_id, sort_order) VALUES (?, ?, ?)");
        imgStmt.bind(1, id);
        imgStmt.bind(2, imageIds[i]);
        imgStmt.bind(3, i);
        imgStmt.exec();
    }

    return Moment{id, authorId, text, imageIds, ts, {}, {}};
}

Moment MockDataStore::loadMoment(int64_t momentId) {
    Moment m;
    m.id = momentId;

    SQLite::Statement stmt(db_,
        "SELECT author_id, text, timestamp FROM moments WHERE id = ?");
    stmt.bind(1, momentId);
    if (!stmt.executeStep()) return m;
    m.authorId = stmt.getColumn(0).getInt64();
    m.text = stmt.getColumn(1).getString();
    m.timestamp = stmt.getColumn(2).getInt64();

    SQLite::Statement imgStmt(db_,
        "SELECT image_id FROM moment_images WHERE moment_id = ? ORDER BY sort_order");
    imgStmt.bind(1, momentId);
    while (imgStmt.executeStep()) {
        m.imageIds.push_back(imgStmt.getColumn(0).getString());
    }

    SQLite::Statement likeStmt(db_,
        "SELECT user_id FROM moment_likes WHERE moment_id = ?");
    likeStmt.bind(1, momentId);
    while (likeStmt.executeStep()) {
        m.likedBy.push_back(likeStmt.getColumn(0).getInt64());
    }

    SQLite::Statement cmtStmt(db_,
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

std::optional<Moment> MockDataStore::findMoment(int64_t momentId) {
    SQLite::Statement stmt(db_,
        "SELECT id FROM moments WHERE id = ?");
    stmt.bind(1, momentId);
    if (!stmt.executeStep()) return std::nullopt;
    return loadMoment(momentId);
}

bool MockDataStore::hasLiked(int64_t momentId, int64_t userId) {
    SQLite::Statement stmt(db_,
        "SELECT 1 FROM moment_likes WHERE moment_id = ? AND user_id = ?");
    stmt.bind(1, momentId);
    stmt.bind(2, userId);
    return stmt.executeStep();
}

void MockDataStore::addLike(int64_t momentId, int64_t userId) {
    SQLite::Statement stmt(db_,
        "INSERT OR IGNORE INTO moment_likes (moment_id, user_id) VALUES (?, ?)");
    stmt.bind(1, momentId);
    stmt.bind(2, userId);
    stmt.exec();
}

int64_t MockDataStore::addComment(int64_t momentId, int64_t authorId,
                                  const std::string& text) {
    auto ts = now();
    SQLite::Statement stmt(db_,
        "INSERT INTO moment_comments (moment_id, author_id, text, timestamp) VALUES (?, ?, ?, ?)");
    stmt.bind(1, momentId);
    stmt.bind(2, authorId);
    stmt.bind(3, text);
    stmt.bind(4, ts);
    stmt.exec();
    return db_.getLastInsertRowid();
}

std::vector<Moment>
MockDataStore::getMoments(std::set<int64_t> const &visibleUserIds,
                          int64_t beforeTs, int limit) {
    std::vector<Moment> result;
    SQLite::Statement stmt(db_,
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
