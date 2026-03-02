#pragma once

#include <wechat/network/GroupService.h>
#include <wechat/network/NetworkTypes.h>
#include <wechat/network/WebSocketClient.h>

#include <QObject>

namespace wechat {
namespace network {

class WsGroupService : public GroupService {

public:
    explicit WsGroupService(WebSocketClient& ws);

    Result<core::Group> createGroup(
        const std::string& token,
        const std::vector<int64_t>& memberIds) override;

    VoidResult dissolveGroup(
        const std::string& token,
        int64_t groupId) override;

    VoidResult addMember(
        const std::string& token,
        int64_t groupId,
        int64_t userId) override;

    VoidResult removeMember(
        const std::string& token,
        int64_t groupId,
        int64_t userId) override;

    Result<std::vector<int64_t>> listMembers(
        const std::string& token,
        int64_t groupId) override;

    Result<std::vector<core::Group>> listMyGroups(
        const std::string& token) override;

private:
    WebSocketClient& ws;
};

} // namespace network
} // namespace wechat
