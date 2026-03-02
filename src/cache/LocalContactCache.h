#pragma once

#include <wechat/network/ContactService.h>
#include <memory>
namespace wechat::network {

class LocalDatabase;

class LocalContactCache : public ContactService {
public:
    explicit LocalContactCache(std::shared_ptr<LocalDatabase> store);

    VoidResult addFriend(
        const std::string& token, int64_t targetUserId) override;
    VoidResult removeFriend(
        const std::string& token, int64_t targetUserId) override;
    Result<std::vector<core::User>> listFriends(
        const std::string& token) override;
    Result<std::vector<core::User>> searchUser(
        const std::string& token, const std::string& keyword) override;

private:
    std::shared_ptr<LocalDatabase> store;
};

} // namespace wechat::network
