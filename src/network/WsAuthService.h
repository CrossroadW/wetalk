#pragma once

#include <wechat/network/AuthService.h>
#include <wechat/network/NetworkTypes.h>
#include <wechat/network/WebSocketClient.h>

#include <QJsonObject>
#include <QString>
#include <optional>

namespace wechat {
namespace network {

/// 基于 WebSocket 的认证服务实现
class WsAuthService : public AuthService {
public:
    explicit WsAuthService(WebSocketClient& ws);

    Result<core::User> registerUser(
        const std::string& username, const std::string& password) override;

    Result<core::User> login(
        const std::string& username, const std::string& password) override;

    VoidResult logout(const std::string& token) override;

    Result<core::User> getCurrentUser(const std::string& token) override;

private:
    WebSocketClient& ws;

    /// 发送请求并等待响应（阻塞式）
    /// @param requestType 请求类型
    /// @param requestData 请求数据
    /// @param timeout 超时时间（毫秒）
    /// @return 响应数据，如果超时或出错返回空
    std::optional<QJsonObject> sendAndWait(
        const QString& requestType,
        const QJsonObject& requestData,
        int timeout = 3000);
};

} // namespace network
} // namespace wechat
