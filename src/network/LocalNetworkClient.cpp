#include "LocalNetworkClient.h"

namespace wechat::network {

LocalNetworkClient::LocalNetworkClient()
    : db(std::make_shared<LocalDatabase>()),
      authService(db),
      chatService(db),
      contactService(db),
      groupService(db),
      momentService(db) {}

AuthService& LocalNetworkClient::auth() { return authService; }
ChatService& LocalNetworkClient::chat() { return chatService; }
ContactService& LocalNetworkClient::contacts() { return contactService; }
GroupService& LocalNetworkClient::groups() { return groupService; }
MomentService& LocalNetworkClient::moments() { return momentService; }

std::unique_ptr<NetworkClient> createMockClient() {
    return std::make_unique<LocalNetworkClient>();
}

} // namespace wechat::network
