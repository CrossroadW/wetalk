#pragma once

#include <cstdint>
#include <vector>

namespace wechat {
namespace core {

struct Group {
    int64_t id = 0;
    int64_t ownerId = 0;
    std::vector<int64_t> memberIds;
};

} // namespace core
} // namespace wechat
