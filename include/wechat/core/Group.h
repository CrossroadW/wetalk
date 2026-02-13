#pragma once

#include <string>
#include <vector>

namespace wechat {
namespace core {

struct Group {
    std::string id;
    std::string ownerId;
    std::vector<std::string> memberIds;
};

} // namespace core
} // namespace wechat
