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
        const std::vector<std::string>& memberIds) override;
    VoidResult dissolveGroup(
        const std::string& token, const std::string& groupId) override;
    VoidResult addMember(
        const std::string& token, const std::string& groupId,
        const std::string& userId) override;
    VoidResult removeMember(
        const std::string& token, const std::string& groupId,
        const std::string& userId) override;
    Result<std::vector<std::string>> listMembers(
        const std::string& token, const std::string& groupId) override;
    Result<std::vector<core::Group>> listMyGroups(
        const std::string& token) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
