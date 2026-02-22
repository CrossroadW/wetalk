#include "wechat/storage/MessageDao.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace wechat {
namespace storage {

// ── JSON 序列化 ──

static json metaToJson(const core::ResourceMeta& m) {
    return {{"size", m.size}, {"filename", m.filename}, {"extra", m.extra}};
}

static core::ResourceMeta jsonToMeta(const json& j) {
    core::ResourceMeta m;
    m.size = j.value("size", std::size_t{0});
    m.filename = j.value("filename", "");
    if (j.contains("extra")) {
        m.extra = j["extra"].get<std::map<std::string, std::string>>();
    }
    return m;
}

static json blockToJson(const core::ContentBlock& block) {
    return std::visit([](auto&& arg) -> json {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return {{"type", 0}};
        } else if constexpr (std::is_same_v<T, core::TextContent>) {
            return {{"type", 1}, {"text", arg.text}};
        } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
            return {{"type", 2},
                    {"resourceId", arg.resourceId},
                    {"resType", static_cast<int>(arg.type)},
                    {"resSubtype", static_cast<int>(arg.subtype)},
                    {"meta", metaToJson(arg.meta)}};
        }
    }, block);
}

static core::ContentBlock jsonToBlock(const json& j) {
    int type = j.value("type", 0);
    switch (type) {
    case 1:
        return core::TextContent{j.value("text", "")};
    case 2: {
        core::ResourceContent rc;
        rc.resourceId = j.value("resourceId", "");
        rc.type = static_cast<core::ResourceType>(j.value("resType", 0));
        rc.subtype = static_cast<core::ResourceSubtype>(j.value("resSubtype", 0));
        if (j.contains("meta")) rc.meta = jsonToMeta(j["meta"]);
        return rc;
    }
    default:
        return std::monostate{};
    }
}

std::string serializeContent(const core::MessageContent& content) {
    json arr = json::array();
    for (const auto& block : content) {
        arr.push_back(blockToJson(block));
    }
    return arr.dump();
}

core::MessageContent deserializeContent(const std::string& str) {
    core::MessageContent content;
    auto arr = json::parse(str, nullptr, false);
    if (arr.is_discarded() || !arr.is_array()) return content;
    for (const auto& item : arr) {
        content.push_back(jsonToBlock(item));
    }
    return content;
}

// ── MessageDao ──

MessageDao::MessageDao(SQLite::Database& db) : db_(db) {}

void MessageDao::insert(const core::Message& msg) {
    SQLite::Statement stmt(db_, R"(
        INSERT OR REPLACE INTO messages
        (id, sender_id, chat_id, reply_to, content_data, timestamp,
         edited_at, revoked, read_count, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    stmt.bind(1, msg.id);
    stmt.bind(2, msg.senderId);
    stmt.bind(3, msg.chatId);
    stmt.bind(4, msg.replyTo);
    stmt.bind(5, serializeContent(msg.content));
    stmt.bind(6, msg.timestamp);
    stmt.bind(7, msg.editedAt);
    stmt.bind(8, msg.revoked ? 1 : 0);
    stmt.bind(9, static_cast<int>(msg.readCount));
    stmt.bind(10, msg.updatedAt);
    stmt.exec();
}

void MessageDao::update(const core::Message& msg) {
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

void MessageDao::remove(int64_t id) {
    SQLite::Statement stmt(db_, "DELETE FROM messages WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}

std::optional<core::Message> MessageDao::findById(int64_t id) {
    SQLite::Statement stmt(db_, R"(
        SELECT id, sender_id, chat_id, reply_to, content_data,
               timestamp, edited_at, revoked, read_count, updated_at
        FROM messages WHERE id = ?
    )");
    stmt.bind(1, id);
    if (!stmt.executeStep()) return std::nullopt;
    return rowToMessage(stmt);
}

std::vector<core::Message> MessageDao::findAfter(
    const std::string& chatId, int64_t afterId, int limit) {
    std::vector<core::Message> result;

    if (afterId == 0) {
        // afterId=0 → 返回最新的 limit 条，升序返回
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
        // afterId>0 → 返回 id > afterId 的前 limit 条，升序返回
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

std::vector<core::Message> MessageDao::findBefore(
    const std::string& chatId, int64_t beforeId, int limit) {
    std::vector<core::Message> result;

    if (beforeId == 0) {
        // beforeId=0 → 返回最早的 limit 条，升序返回
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
        // beforeId>0 → 返回 id < beforeId 的最后 limit 条，升序返回
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

std::vector<core::Message> MessageDao::findUpdatedAfter(
    const std::string& chatId, int64_t startId, int64_t endId,
    int64_t since, int limit) {
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
    stmt.bind(4, since);
    stmt.bind(5, limit);
    while (stmt.executeStep()) {
        result.push_back(rowToMessage(stmt));
    }
    return result;
}

core::Message MessageDao::rowToMessage(SQLite::Statement& stmt) {
    core::Message msg;
    msg.id = stmt.getColumn(0).getInt64();
    msg.senderId = stmt.getColumn(1).getString();
    msg.chatId = stmt.getColumn(2).getString();
    msg.replyTo = stmt.getColumn(3).getInt64();
    msg.content = deserializeContent(stmt.getColumn(4).getString());
    msg.timestamp = stmt.getColumn(5).getInt64();
    msg.editedAt = stmt.getColumn(6).getInt64();
    msg.revoked = stmt.getColumn(7).getInt() != 0;
    msg.readCount = static_cast<uint32_t>(stmt.getColumn(8).getInt());
    msg.updatedAt = stmt.getColumn(9).getInt64();
    return msg;
}

void MessageDao::revoke(int64_t id, int64_t now) {
    SQLite::Statement stmt(db_, R"(
        UPDATE messages SET revoked = 1, updated_at = ? WHERE id = ?
    )");
    stmt.bind(1, now);
    stmt.bind(2, id);
    stmt.exec();
}

void MessageDao::editContent(int64_t id,
                             const core::MessageContent& content, int64_t now) {
    SQLite::Statement stmt(db_, R"(
        UPDATE messages SET content_data = ?, edited_at = ?, updated_at = ? WHERE id = ?
    )");
    stmt.bind(1, serializeContent(content));
    stmt.bind(2, now);
    stmt.bind(3, now);
    stmt.bind(4, id);
    stmt.exec();
}

void MessageDao::updateReadCount(int64_t id, uint32_t readCount,
                                 int64_t now) {
    SQLite::Statement stmt(db_, R"(
        UPDATE messages SET read_count = ?, updated_at = ? WHERE id = ?
    )");
    stmt.bind(1, static_cast<int>(readCount));
    stmt.bind(2, now);
    stmt.bind(3, id);
    stmt.exec();
}

} // namespace storage
} // namespace wechat
