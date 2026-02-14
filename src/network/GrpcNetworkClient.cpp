#include "GrpcNetworkClient.h"

#include "GrpcAuthService.h"
#include "MockAuthService.h"
#include "MockChatService.h"
#include "MockContactService.h"
#include "MockGroupService.h"
#include "MockMomentService.h"
#include "auth.pb.h"
#include "auth.grpc.pb.h"
namespace wechat {
namespace network {

GrpcNetworkClient::GrpcNetworkClient(const std::string& serverAddress)
    : store(std::make_shared<MockDataStore>()),
      authService(std::make_unique<GrpcAuthService>(serverAddress)),
      chatService(std::make_unique<MockChatService>(store)),
      contactService(std::make_unique<MockContactService>(store)),
      groupService(std::make_unique<MockGroupService>(store)),
      momentService(std::make_unique<MockMomentService>(store)) {}

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
