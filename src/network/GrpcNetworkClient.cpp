#include "GrpcNetworkClient.h"

#include "GrpcAuthService.h"
#include "MockAuthService.h"
#include "MockChatService.h"
#include "MockContactService.h"
#include "MockGroupService.h"
#include "MockMomentService.h"

namespace wechat {
namespace network {

GrpcNetworkClient::GrpcNetworkClient(const std::string& serverAddress)
    : store(std::make_shared<MockDataStore>()),
      authService(serverAddress),
      chatService(store),
      contactService(store),
      groupService(store),
      momentService(store) {}

AuthService& GrpcNetworkClient::auth() {
    return authService;
}

ChatService& GrpcNetworkClient::chat() {
    return chatService;
}

ContactService& GrpcNetworkClient::contacts() {
    return contactService;
}

GroupService& GrpcNetworkClient::groups() {
    return groupService;
}

MomentService& GrpcNetworkClient::moments() {
    return momentService;
}

std::unique_ptr<NetworkClient> createGrpcClient(
    const std::string& serverAddress) {
    return std::make_unique<GrpcNetworkClient>(serverAddress);
}

} // namespace network
} // namespace wechat
