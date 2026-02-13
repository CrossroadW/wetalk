#include "wechat/storage/UserDao.h"

namespace wechat {
namespace storage {

UserDao::UserDao(SQLite::Database& db) : db_(db) {}

void UserDao::insert(const core::User& user) {
    SQLite::Statement stmt(db_,
        "INSERT OR REPLACE INTO users (id) VALUES (?)");
    stmt.bind(1, user.id);
    stmt.exec();
}

void UserDao::remove(const std::string& id) {
    SQLite::Statement stmt(db_, "DELETE FROM users WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}

std::optional<core::User> UserDao::findById(const std::string& id) {
    SQLite::Statement stmt(db_, "SELECT id FROM users WHERE id = ?");
    stmt.bind(1, id);
    if (stmt.executeStep()) {
        return core::User{stmt.getColumn(0).getString()};
    }
    return std::nullopt;
}

std::vector<core::User> UserDao::findAll() {
    std::vector<core::User> result;
    SQLite::Statement stmt(db_, "SELECT id FROM users");
    while (stmt.executeStep()) {
        result.push_back(core::User{stmt.getColumn(0).getString()});
    }
    return result;
}

} // namespace storage
} // namespace wechat
