#pragma once

#include "wechat/core/User.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <string>
#include <vector>

namespace wechat {
namespace storage {

class UserDao {
public:
    explicit UserDao(SQLite::Database& db);

    void insert(const core::User& user);
    void remove(const std::string& id);
    std::optional<core::User> findById(const std::string& id);
    std::vector<core::User> findAll();

private:
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
