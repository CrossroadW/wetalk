#pragma once

#include <wechat/network/AuthService.h>
#include <wechat/network/ChatService.h>
#include <wechat/network/ContactService.h>
#include <wechat/network/GroupService.h>
#include <wechat/network/MomentService.h>

#include <memory>

namespace wechat::network {

/// 网络客户端抽象入口
/// 上层模块通过此接口获取各 Service，不关心底层是 Mock 还是 gRPC
class NetworkClient {
public:
    virtual ~NetworkClient() = default;

    virtual AuthService& auth() = 0;
    virtual ChatService& chat() = 0;
    virtual ContactService& contacts() = 0;
    virtual GroupService& groups() = 0;
    virtual MomentService& moments() = 0;
};

/// 创建 Mock 实现（阶段 1 使用）
std::unique_ptr<NetworkClient> createMockClient();

} // namespace wechat::network
