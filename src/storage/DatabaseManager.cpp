#include "wechat/storage/DatabaseManager.h"

namespace wechat {
namespace storage {

DatabaseManager::DatabaseManager(const std::string& dbPath)
    : db_(std::make_unique<SQLite::Database>(
          dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)) {
    db_->exec("PRAGMA journal_mode=WAL");
    db_->exec("PRAGMA foreign_keys=ON");
}

SQLite::Database& DatabaseManager::db() { return *db_; }

void DatabaseManager::initSchema() {
    db_->exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT
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
            id INTEGER PRIMARY KEY,
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
    )");
}

} // namespace storage
} // namespace wechat
