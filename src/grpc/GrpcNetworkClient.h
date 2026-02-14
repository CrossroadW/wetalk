#pragma once

#include <wechat/network/NetworkClient.h>
#include "MockDataStore.h"
#include "GrpcAuthService.h"
#include "MockChatService.h"
#include "MockContactService.h"
#include "MockGroupService.h"
#include "MockMomentService.h"
#include "auth.pb.h"
#include "auth.grpc.pb.h"
#include <memory>
#include <string>
using namespace wechat::auth;
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
    std::unique_ptr<MockChatService> chatService;
    std::unique_ptr<MockContactService> contactService;
    std::unique_ptr<MockGroupService> groupService;
    std::unique_ptr<MockMomentService> momentService;
};

/// 创建 gRPC 客户端
/// TODO: 当后端就绪时启用
std::unique_ptr<NetworkClient> createGrpcClient(
    const std::string& serverAddress = "localhost:50051");

} // namespace network
} // namespace wechat
