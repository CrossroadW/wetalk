#pragma once

#include <wechat/network/NetworkClient.h>
#include <wechat/network/WebSocketClient.h>

#include <memory>

namespace wechat::network {

class WsAuthService;
class WsChatService;
class WsContactService;
class WsGroupService;
class WsMomentService;

/// 基于 WebSocket 的 NetworkClient 实现
/// 用于与真实后端服务器通信
class WsNetworkClient : public NetworkClient {
public:
    /// 构造函数
    /// @param wsUrl WebSocket 服务器地址（如 ws://localhost:8000/ws）
    /// @param connectTimeout 连接超时时间（毫秒），默认 5000ms
    explicit WsNetworkClient(const QString& wsUrl, int connectTimeout = 5000);
    ~WsNetworkClient() override;

    AuthService& auth() override;
    ChatService& chat() override;
    ContactService& contacts() override;
    GroupService& groups() override;
    MomentService& moments() override;
    WebSocketClient* ws() override { return wsClient_.get(); }

    /// 检查是否已连接
    bool isConnected() const;

private:
    std::unique_ptr<WebSocketClient> wsClient_;
    std::unique_ptr<WsAuthService> authService;
    std::unique_ptr<WsChatService> chatService;
    std::unique_ptr<WsContactService> contactService;
    std::unique_ptr<WsGroupService> groupService;
    std::unique_ptr<WsMomentService> momentService;
};

/// 创建 WebSocket 客户端（连接到真实后端）
/// @param wsUrl WebSocket 服务器地址
std::unique_ptr<NetworkClient> createWsClient(const QString& wsUrl);

} // namespace wechat::network
