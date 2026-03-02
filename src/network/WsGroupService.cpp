#include "WsGroupService.h"

namespace wechat {
namespace network {

WsGroupService::WsGroupService(WebSocketClient& ws)
    :  ws(ws) {}

Result<core::Group> WsGroupService::createGroup(
    const std::string& token,
    const std::vector<int64_t>& memberIds) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsGroupService::dissolveGroup(
    const std::string& token,
    int64_t groupId) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsGroupService::addMember(
    const std::string& token,
    int64_t groupId,
    int64_t userId) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsGroupService::removeMember(
    const std::string& token,
    int64_t groupId,
    int64_t userId) {
    return std::unexpected("Not implemented yet");
}

Result<std::vector<int64_t>> WsGroupService::listMembers(
    const std::string& token,
    int64_t groupId) {
    return std::unexpected("Not implemented yet");
}

Result<std::vector<core::Group>> WsGroupService::listMyGroups(
    const std::string& token) {
    return std::unexpected("Not implemented yet");
}

} // namespace network
} // namespace wechat
