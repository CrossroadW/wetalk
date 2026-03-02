#include "WsContactService.h"

namespace wechat {
namespace network {

WsContactService::WsContactService(WebSocketClient& ws)
    : ws(ws) {}

VoidResult WsContactService::addFriend(
    const std::string& token,
    int64_t targetUserId) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsContactService::removeFriend(
    const std::string& token,
    int64_t targetUserId) {
    return std::unexpected("Not implemented yet");
}

Result<std::vector<core::User>> WsContactService::listFriends(
    const std::string& token) {
    return std::unexpected("Not implemented yet");
}

Result<std::vector<core::User>> WsContactService::searchUser(
    const std::string& token,
    const std::string& keyword) {
    return std::unexpected("Not implemented yet");
}

} // namespace network
} // namespace wechat
