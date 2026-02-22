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

} // namespace storage
} // namespace wechat
