#pragma once

#include <wechat/network/AuthService.h>
#include <wechat/network/ChatService.h>
#include <wechat/network/ContactService.h>
#include <wechat/network/GroupService.h>
#include <wechat/network/MomentService.h>

#include <QString>
#include <memory>

namespace wechat::network {

/// 网络客户端抽象入口
/// 上层模块通过此接口获取各 Service
class NetworkClient {
public:
    virtual ~NetworkClient() = default;

    virtual AuthService& auth() = 0;
    virtual ChatService& chat() = 0;
    virtual ContactService& contacts() = 0;
    virtual GroupService& groups() = 0;
    virtual MomentService& moments() = 0;
};

/// 创建本地缓存实现（用于测试和离线模式）
std::unique_ptr<NetworkClient> createMockClient();

/// 创建 WebSocket 客户端（连接到真实后端）
/// @param wsUrl WebSocket 服务器地址，默认 ws://localhost:8000/ws
/// @throws std::runtime_error 如果连接失败
std::unique_ptr<NetworkClient> createWsClient(
    const QString& wsUrl = "ws://localhost:8000/ws");

} // namespace wechat::network

