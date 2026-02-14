#pragma once

#include <wechat/network/ContactService.h>
#include <memory>
namespace wechat::network {

class MockDataStore;

class MockContactService : public ContactService {
public:
    explicit MockContactService(std::shared_ptr<MockDataStore> store);

    VoidResult addFriend(
        const std::string& token, const std::string& targetUserId) override;
    VoidResult removeFriend(
        const std::string& token, const std::string& targetUserId) override;
    Result<std::vector<core::User>> listFriends(
        const std::string& token) override;
    Result<std::vector<core::User>> searchUser(
        const std::string& token, const std::string& keyword) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
