#pragma once

#include <wechat/network/NetworkClient.h>

#include "../cache/LocalAuthCache.h"
#include "../cache/LocalChatCache.h"
#include "../cache/LocalContactCache.h"
#include "../cache/LocalDatabase.h"
#include "../cache/LocalGroupCache.h"
#include "../cache/LocalMomentCache.h"

#include <memory>

namespace wechat::network {

/// 纯本地缓存实现的 NetworkClient（用于测试和离线模式）
class LocalNetworkClient : public NetworkClient {
public:
    LocalNetworkClient();

    AuthService& auth() override;
    ChatService& chat() override;
    ContactService& contacts() override;
    GroupService& groups() override;
    MomentService& moments() override;
    WebSocketClient* ws() override { return nullptr; }

private:
    std::shared_ptr<LocalDatabase> db;
    LocalAuthCache authService;
    LocalChatCache chatService;
    LocalContactCache contactService;
    LocalGroupCache groupService;
    LocalMomentCache momentService;
};

/// 创建本地缓存实现（用于测试）
std::unique_ptr<NetworkClient> createMockClient();

} // namespace wechat::network
