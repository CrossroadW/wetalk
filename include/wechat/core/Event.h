#pragma once

#include <variant>

namespace wechat::core {

// TODO: 替换为实际的事件类型
struct PlaceholderEvent {};

using Event = std::variant<std::monostate,PlaceholderEvent>;

} // namespace wechat::core
