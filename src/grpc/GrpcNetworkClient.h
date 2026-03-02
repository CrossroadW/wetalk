#pragma once

#include <wechat/network/NetworkClient.h>
#include "../cache/LocalDatabase.h"
#include "GrpcAuthService.h"
#include "../cache/LocalChatCache.h"
#include "../cache/LocalContactCache.h"
#include "../cache/LocalGroupCache.h"
#include "../cache/LocalMomentCache.h"
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
    std::shared_ptr<LocalDatabase> store;
    std::unique_ptr<GrpcAuthService> authService;
    std::unique_ptr<LocalChatCache> chatService;
    std::unique_ptr<LocalContactCache> contactService;
    std::unique_ptr<LocalGroupCache> groupService;
    std::unique_ptr<LocalMomentCache> momentService;
};

/// 创建 gRPC 客户端
/// TODO: 当后端就绪时启用
std::unique_ptr<NetworkClient> createGrpcClient(
    const std::string& serverAddress = "localhost:50051");

} // namespace network
} // namespace wechat
