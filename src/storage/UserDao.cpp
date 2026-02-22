#include "wechat/storage/UserDao.h"

namespace wechat {
namespace storage {

UserDao::UserDao(SQLite::Database& db) : db_(db) {}

int64_t UserDao::insert(const core::User& user) {
    SQLite::Statement stmt(db_, "INSERT INTO users DEFAULT VALUES");
    stmt.exec();
    return db_.getLastInsertRowid();
}

void UserDao::remove(int64_t id) {
    SQLite::Statement stmt(db_, "DELETE FROM users WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}

std::optional<core::User> UserDao::findById(int64_t id) {
    SQLite::Statement stmt(db_, "SELECT id FROM users WHERE id = ?");
    stmt.bind(1, id);
    if (stmt.executeStep()) {
        return core::User{stmt.getColumn(0).getInt64()};
    }
    return std::nullopt;
}

std::vector<core::User> UserDao::findAll() {
    std::vector<core::User> result;
    SQLite::Statement stmt(db_, "SELECT id FROM users");
    while (stmt.executeStep()) {
        result.push_back(core::User{stmt.getColumn(0).getInt64()});
    }
    return result;
}

} // namespace storage
} // namespace wechat
