#pragma once

#include <wechat/network/GroupService.h>

#include <memory>
namespace wechat::network {

class MockDataStore;

class MockGroupService : public GroupService {
public:
    explicit MockGroupService(std::shared_ptr<MockDataStore> store);

    Result<core::Group> createGroup(
        const std::string& token,
        const std::vector<int64_t>& memberIds) override;
    VoidResult dissolveGroup(
        const std::string& token, int64_t groupId) override;
    VoidResult addMember(
        const std::string& token, int64_t groupId,
        int64_t userId) override;
    VoidResult removeMember(
        const std::string& token, int64_t groupId,
        int64_t userId) override;
    Result<std::vector<int64_t>> listMembers(
        const std::string& token, int64_t groupId) override;
    Result<std::vector<core::Group>> listMyGroups(
        const std::string& token) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
