#include "MessageStore.h"
#include "MessageJson.h"

namespace wechat {
namespace cache {

MessageStore::MessageStore(const std::string& dbPath)
    : db_(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    db_.exec("PRAGMA journal_mode=WAL");
    db_.exec("PRAGMA synchronous=NORMAL");
    db_.exec("PRAGMA cache_size=-8000");
    initSchema();
}

void MessageStore::initSchema() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id          INTEGER PRIMARY KEY,
            chat_seq    INTEGER NOT NULL DEFAULT 0,
            sender_id   INTEGER NOT NULL DEFAULT 0,
            chat_id     INTEGER NOT NULL,
            reply_to    INTEGER NOT NULL DEFAULT 0,
            content_json TEXT NOT NULL DEFAULT '[]',
            timestamp   INTEGER NOT NULL DEFAULT 0,
            edited_at   INTEGER NOT NULL DEFAULT 0,
            revoked     INTEGER NOT NULL DEFAULT 0,
            read_count  INTEGER NOT NULL DEFAULT 0,
            updated_at  INTEGER NOT NULL DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_msg_chat_seq
            ON messages(chat_id, chat_seq);
        CREATE INDEX IF NOT EXISTS idx_msg_updated
            ON messages(chat_id, updated_at);
    )");
}

core::Message MessageStore::rowToMessage(SQLite::Statement& stmt) const {
    core::Message msg;
    msg.id        = stmt.getColumn(0).getInt64();
    msg.chatSeq   = stmt.getColumn(1).getInt();
    msg.senderId  = stmt.getColumn(2).getInt64();
    msg.chatId    = stmt.getColumn(3).getInt64();
    msg.replyTo   = stmt.getColumn(4).getInt64();
    msg.content   = detail::deserializeContent(stmt.getColumn(5).getString());
    msg.timestamp = stmt.getColumn(6).getInt64();
    msg.editedAt  = stmt.getColumn(7).getInt64();
    msg.revoked   = stmt.getColumn(8).getInt() != 0;
    msg.readCount = static_cast<uint32_t>(stmt.getColumn(9).getInt());
    msg.updatedAt = stmt.getColumn(10).getInt64();
    return msg;
}

void MessageStore::upsertMessage(const core::Message& msg) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO messages
            (id, chat_seq, sender_id, chat_id, reply_to, content_json,
             timestamp, edited_at, revoked, read_count, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?)
        ON CONFLICT(id) DO UPDATE SET
            chat_seq     = excluded.chat_seq,
            content_json = excluded.content_json,
            edited_at    = excluded.edited_at,
            revoked      = excluded.revoked,
            read_count   = excluded.read_count,
            updated_at   = excluded.updated_at
    )");
    stmt.bind(1,  msg.id);
    stmt.bind(2,  msg.chatSeq);
    stmt.bind(3,  msg.senderId);
    stmt.bind(4,  msg.chatId);
    stmt.bind(5,  msg.replyTo);
    stmt.bind(6,  detail::serializeContent(msg.content));
    stmt.bind(7,  msg.timestamp);
    stmt.bind(8,  msg.editedAt);
    stmt.bind(9,  msg.revoked ? 1 : 0);
    stmt.bind(10, static_cast<int>(msg.readCount));
    stmt.bind(11, msg.updatedAt);
    stmt.exec();
}

void MessageStore::upsertMessages(const std::vector<core::Message>& msgs) {
    SQLite::Transaction tx(db_);
    for (const auto& msg : msgs) upsertMessage(msg);
    tx.commit();
}

std::vector<core::Message> MessageStore::loadLatest(int64_t chatId, int limit) {
    SQLite::Statement stmt(db_, R"(
        SELECT * FROM (
            SELECT id, chat_seq, sender_id, chat_id, reply_to, content_json,
                   timestamp, edited_at, revoked, read_count, updated_at
            FROM messages WHERE chat_id = ?
            ORDER BY chat_seq DESC LIMIT ?
        ) sub ORDER BY chat_seq ASC
    )");
    stmt.bind(1, chatId);
    stmt.bind(2, limit);
    std::vector<core::Message> result;
    while (stmt.executeStep()) result.push_back(rowToMessage(stmt));
    return result;
}

std::vector<core::Message> MessageStore::loadBefore(
    int64_t chatId, int32_t chatSeq, int limit) {

    SQLite::Statement stmt(db_, R"(
        SELECT * FROM (
            SELECT id, chat_seq, sender_id, chat_id, reply_to, content_json,
                   timestamp, edited_at, revoked, read_count, updated_at
            FROM messages
            WHERE chat_id = ? AND chat_seq < ?
            ORDER BY chat_seq DESC LIMIT ?
        ) sub ORDER BY chat_seq ASC
    )");
    stmt.bind(1, chatId);
    stmt.bind(2, chatSeq);
    stmt.bind(3, limit);
    std::vector<core::Message> result;
    while (stmt.executeStep()) result.push_back(rowToMessage(stmt));
    return result;
}

std::vector<core::Message> MessageStore::loadAfter(
    int64_t chatId, int32_t chatSeq, int limit) {

    SQLite::Statement stmt(db_, R"(
        SELECT id, chat_seq, sender_id, chat_id, reply_to, content_json,
               timestamp, edited_at, revoked, read_count, updated_at
        FROM messages
        WHERE chat_id = ? AND chat_seq > ?
        ORDER BY chat_seq ASC LIMIT ?
    )");
    stmt.bind(1, chatId);
    stmt.bind(2, chatSeq);
    stmt.bind(3, limit);
    std::vector<core::Message> result;
    while (stmt.executeStep()) result.push_back(rowToMessage(stmt));
    return result;
}

std::vector<core::Message> MessageStore::loadAround(
    int64_t chatId, int32_t chatSeq, int radius) {

    SQLite::Statement stmt(db_, R"(
        SELECT id, chat_seq, sender_id, chat_id, reply_to, content_json,
               timestamp, edited_at, revoked, read_count, updated_at
        FROM messages
        WHERE chat_id = ? AND chat_seq >= ? AND chat_seq <= ?
        ORDER BY chat_seq ASC
    )");
    stmt.bind(1, chatId);
    stmt.bind(2, chatSeq - radius);
    stmt.bind(3, chatSeq + radius);
    std::vector<core::Message> result;
    while (stmt.executeStep()) result.push_back(rowToMessage(stmt));
    return result;
}

std::vector<core::Message> MessageStore::loadUpdated(
    int64_t chatId, int64_t maxUpdatedAt, int limit) {

    SQLite::Statement stmt(db_, R"(
        SELECT id, chat_seq, sender_id, chat_id, reply_to, content_json,
               timestamp, edited_at, revoked, read_count, updated_at
        FROM messages
        WHERE chat_id = ? AND updated_at > ?
        ORDER BY updated_at ASC LIMIT ?
    )");
    stmt.bind(1, chatId);
    stmt.bind(2, maxUpdatedAt);
    stmt.bind(3, limit);
    std::vector<core::Message> result;
    while (stmt.executeStep()) result.push_back(rowToMessage(stmt));
    return result;
}

int64_t MessageStore::maxUpdatedAt(int64_t chatId) const {
    SQLite::Statement stmt(db_,
        "SELECT MAX(updated_at) FROM messages WHERE chat_id = ?");
    stmt.bind(1, chatId);
    if (stmt.executeStep() && !stmt.getColumn(0).isNull())
        return stmt.getColumn(0).getInt64();
    return 0;
}

} // namespace cache
} // namespace wechat
