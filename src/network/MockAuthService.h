#pragma once

#include <wechat/network/AuthService.h>

#include <memory>

namespace wechat::network {

class MockDataStore;

class MockAuthService : public AuthService {
public:
    explicit MockAuthService(std::shared_ptr<MockDataStore> store);

    Result<LoginResponse> registerUser(
        const std::string& username, const std::string& password) override;
    Result<LoginResponse> login(
        const std::string& username, const std::string& password) override;
    VoidResult logout(const std::string& token) override;
    Result<core::User> getCurrentUser(const std::string& token) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
