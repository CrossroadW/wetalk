#pragma once

#include <wechat/network/ContactService.h>
#include <wechat/network/NetworkTypes.h>
#include <wechat/network/WebSocketClient.h>

namespace wechat {
namespace network {

class WsContactService : public ContactService {
public:
    explicit WsContactService(WebSocketClient& ws);

    VoidResult addFriend(
        const std::string& token,
        int64_t targetUserId) override;

    VoidResult removeFriend(
        const std::string& token,
        int64_t targetUserId) override;

    Result<std::vector<core::User>> listFriends(
        const std::string& token) override;

    Result<std::vector<core::User>> searchUser(
        const std::string& token,
        const std::string& keyword) override;

private:
    WebSocketClient& ws;
};

} // namespace network
} // namespace wechat
