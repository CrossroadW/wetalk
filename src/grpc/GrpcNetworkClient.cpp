#include "GrpcNetworkClient.h"

#include "GrpcAuthService.h"
#include "../cache/LocalAuthCache.h"
#include "../cache/LocalChatCache.h"
#include "../cache/LocalContactCache.h"
#include "../cache/LocalGroupCache.h"
#include "../cache/LocalMomentCache.h"
#include "auth.pb.h"
#include "auth.grpc.pb.h"
namespace wechat {
namespace network {

GrpcNetworkClient::GrpcNetworkClient(const std::string& serverAddress)
    : store(std::make_shared<LocalDatabase>()),
      authService(std::make_unique<GrpcAuthService>(serverAddress)),
      chatService(std::make_unique<LocalChatCache>(store)),
      contactService(std::make_unique<LocalContactCache>(store)),
      groupService(std::make_unique<LocalGroupCache>(store)),
      momentService(std::make_unique<LocalMomentCache>(store)) {}

AuthService& GrpcNetworkClient::auth() {
    return *authService;
}

ChatService& GrpcNetworkClient::chat() {
    return *chatService;
}

ContactService& GrpcNetworkClient::contacts() {
    return *contactService;
}

GroupService& GrpcNetworkClient::groups() {
    return *groupService;
}

MomentService& GrpcNetworkClient::moments() {
    return *momentService;
}

std::unique_ptr<NetworkClient> createGrpcClient(
    const std::string& serverAddress) {
    return std::make_unique<GrpcNetworkClient>(serverAddress);
}

} // namespace network
} // namespace wechat
