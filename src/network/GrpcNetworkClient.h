#pragma once

#include "MockDataStore.h"

#include <memory>
#include <string>

namespace wechat {
namespace network {

// 前向声明
class GrpcAuthService;

/// gRPC 网络客户端实现
/// 需要后端服务运行才能使用
class GrpcNetworkClient : public NetworkClient {
public:
    explicit GrpcNetworkClient(const std::string& serverAddress =
                                  "localhost:50051");

    AuthService& auth() override;
    ChatService& chat() override;
    ContactService& contacts() override;
    GroupService& groups() override;
    MomentService& moments() override;

private:
    std::shared_ptr<MockDataStore> store;
    std::unique_ptr<GrpcAuthService> authService;
    std::unique_ptr<ChatService> chatService;
    std::unique_ptr<ContactService> contactService;
    std::unique_ptr<GroupService> groupService;
    std::unique_ptr<MomentService> momentService;
};

/// 创建 gRPC 客户端
/// TODO: 当后端就绪时启用
std::unique_ptr<NetworkClient> createGrpcClient(
    const std::string& serverAddress = "localhost:50051");

} // namespace network
} // namespace wechat
