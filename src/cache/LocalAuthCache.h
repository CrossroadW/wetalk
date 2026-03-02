#pragma once

#include <wechat/network/AuthService.h>

#include <memory>
namespace wechat::network {

class LocalDatabase;

class LocalAuthCache : public AuthService {
public:
    explicit LocalAuthCache(std::shared_ptr<LocalDatabase> store);

    Result<core::User> registerUser(
        const std::string& username, const std::string& password) override;
    Result<core::User> login(
        const std::string& username, const std::string& password) override;
    VoidResult logout(const std::string& token) override;
    Result<core::User> getCurrentUser(const std::string& token) override;

private:
    std::shared_ptr<LocalDatabase> store;
};

} // namespace wechat::network
