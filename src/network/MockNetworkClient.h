#pragma once

#include <wechat/network/NetworkClient.h>

#include "MockAuthService.h"
#include "MockChatService.h"
#include "MockContactService.h"
#include "MockDataStore.h"
#include "MockGroupService.h"
#include "MockMomentService.h"

#include <memory>

namespace wechat::network {

class MockNetworkClient : public NetworkClient {
public:
    MockNetworkClient();

    AuthService& auth() override;
    ChatService& chat() override;
    ContactService& contacts() override;
    GroupService& groups() override;
    MomentService& moments() override;

private:
    std::shared_ptr<MockDataStore> store;
    MockAuthService authService;
    MockChatService chatService;
    MockContactService contactService;
    MockGroupService groupService;
    MockMomentService momentService;
};

} // namespace wechat::network
