#pragma once

#include <wechat/core/User.h>
#include <wechat/network/NetworkTypes.h>

#include <optional>
#include <string>

namespace wechat::network {

/// 登录响应
struct LoginResponse {
    std::string userId;
    std::string token;
};

/// 认证服务接口
class AuthService {
public:
    virtual ~AuthService() = default;

    /// 注册
    virtual Result<LoginResponse> registerUser(
        const std::string& username, const std::string& password) = 0;

    /// 登录
    virtual Result<LoginResponse> login(
        const std::string& username, const std::string& password) = 0;

    /// 登出
    virtual VoidResult logout(const std::string& token) = 0;

    /// 通过 token 获取当前用户
    virtual Result<core::User> getCurrentUser(const std::string& token) = 0;
};

} // namespace wechat::network
