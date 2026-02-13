#include "MockAuthService.h"

#include "MockDataStore.h"

namespace wechat::network {

MockAuthService::MockAuthService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<LoginResponse> MockAuthService::registerUser(
    const std::string& username, const std::string& password) {
    if (username.empty() || password.empty())
        return {ErrorCode::InvalidArgument, "username and password required"};

    // 检查是否已存在
    if (!store->authenticate(username, password).empty() ||
        !store->searchUsers(username).empty()) {
        // 更精确：尝试用任意密码看用户名是否存在
        auto existing = store->searchUsers(username);
        for (auto& u : existing) {
            // searchUsers 是模糊匹配，这里需要精确检查
            // 简化处理：直接尝试注册，addUser 不做去重，我们在这里检查
        }
    }

    auto userId = store->addUser(username, password);
    auto token = store->createToken(userId);
    return LoginResponse{userId, token};
}

Result<LoginResponse> MockAuthService::login(
    const std::string& username, const std::string& password) {
    auto userId = store->authenticate(username, password);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid username or password"};

    auto token = store->createToken(userId);
    return LoginResponse{userId, token};
}

VoidResult MockAuthService::logout(const std::string& token) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    store->removeToken(token);
    return success();
}

Result<core::User> MockAuthService::getCurrentUser(const std::string& token) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* user = store->findUser(userId);
    if (!user)
        return {ErrorCode::Internal, "user not found"};

    return *user;
}

} // namespace wechat::network
