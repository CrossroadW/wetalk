#pragma once

#include <wechat/core/User.h>
#include <wechat/network/NetworkTypes.h>

#include <string>

namespace wechat::network {

/// 认证服务接口
class AuthService {
public:
    struct QRLoginInitResult {
        std::string sessionId;
        std::string qrUrl;
    };

    virtual ~AuthService() = default;

    /// 注册
    virtual Result<core::User> registerUser(
        const std::string& username, const std::string& password) = 0;

    /// 登录
    virtual Result<core::User> login(
        const std::string& username, const std::string& password) = 0;

    /// 登出
    virtual VoidResult logout(const std::string& token) = 0;

    /// 通过 token 获取当前用户
    virtual Result<core::User> getCurrentUser(const std::string& token) = 0;

    /// 发起二维码登录，返回会话ID和二维码URL
    virtual Result<QRLoginInitResult> startQRLogin() = 0;
};

} // namespace wechat::network
