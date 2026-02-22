#pragma once

#include <cstdint>
#include <string>

namespace wechat {
namespace core {

struct User {
    int64_t id = 0;
    std::string username;
    std::string password;
    std::string token;
};

} // namespace core
} // namespace wechat
