#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace wechat::network {

// ── 错误码 ──

enum class ErrorCode : uint8_t {
    Ok = 0,
    InvalidArgument,
    NotFound,
    AlreadyExists,
    Unauthorized,
    PermissionDenied,
    Internal,
    Unavailable,
    Timeout,
};

// ── 错误信息 ──

struct Error {
    ErrorCode code;
    std::string message;
};

// ── Result<T>：成功返回 T，失败返回 Error ──

template <typename T>
class Result {
    std::variant<T, Error> data;

public:
    Result(T value) : data(std::move(value)) {}
    Result(Error err) : data(std::move(err)) {}
    Result(ErrorCode code, std::string msg)
        : data(Error{code, std::move(msg)}) {}

    [[nodiscard]] bool ok() const { return std::holds_alternative<T>(data); }
    explicit operator bool() const { return ok(); }

    const T& value() const& { return std::get<T>(data); }
    T& value() & { return std::get<T>(data); }
    T&& value() && { return std::get<T>(std::move(data)); }

    const Error& error() const& { return std::get<Error>(data); }
};

// ── Result<void> 特化 ──

struct Unit {};

using VoidResult = Result<Unit>;

inline VoidResult success() { return Unit{}; }

} // namespace wechat::network
