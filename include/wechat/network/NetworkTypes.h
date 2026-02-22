#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string>

namespace wechat {
namespace network {

// ── 错误码 → 固定字符串映射 ──

enum class Err : uint8_t {
    InvalidArgument,
    NotFound,
    AlreadyExists,
    Unauthorized,
    PermissionDenied,
    Internal,
    Unavailable,
    Timeout,
    Count_  // 哨兵，不要使用
};

inline constexpr std::array kErrMessages = {
    "invalid argument",
    "not found",
    "already exists",
    "unauthorized",
    "permission denied",
    "internal error",
    "service unavailable",
    "timeout",
};

/// 枚举 → 字符串
inline std::string errMsg(Err e) {
    auto i = static_cast<uint8_t>(e);
    if (i < kErrMessages.size()) return std::string(kErrMessages[i]);
    return "unknown error";
}

/// 快捷构造 unexpected：errMsg(Err) 或自定义字符串
inline std::unexpected<std::string> fail(Err e) {
    return std::unexpected(errMsg(e));
}
inline std::unexpected<std::string> fail(std::string msg) {
    return std::unexpected(std::move(msg));
}

// ── Result 类型别名 ──

template <typename T>
using Result = std::expected<T, std::string>;

using VoidResult = std::expected<void, std::string>;

inline VoidResult success() { return {}; }

} // namespace network
} // namespace wechat
