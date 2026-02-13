#include "MockNetworkClient.h"

namespace wechat::network {

MockNetworkClient::MockNetworkClient()
    : store(std::make_shared<MockDataStore>()),
      authService(store),
      chatService(store),
      contactService(store),
      groupService(store),
      momentService(store) {}

AuthService& MockNetworkClient::auth() { return authService; }
ChatService& MockNetworkClient::chat() { return chatService; }
ContactService& MockNetworkClient::contacts() { return contactService; }
GroupService& MockNetworkClient::groups() { return groupService; }
MomentService& MockNetworkClient::moments() { return momentService; }

std::unique_ptr<NetworkClient> createMockClient() {
    return std::make_unique<MockNetworkClient>();
}

} // namespace wechat::network
