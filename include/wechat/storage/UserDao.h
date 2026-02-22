#pragma once

#include "wechat/core/User.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace wechat {
namespace storage {

class UserDao {
public:
    explicit UserDao(SQLite::Database& db);

    void insert(const core::User& user);
    void remove(int64_t id);
    std::optional<core::User> findById(int64_t id);
    std::vector<core::User> findAll();

private:
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
