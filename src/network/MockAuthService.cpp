#include "MockAuthService.h"

#include "MockDataStore.h"

namespace wechat::network {

MockAuthService::MockAuthService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<LoginResponse> MockAuthService::registerUser(
    const std::string& username, const std::string& password) {
    if (username.empty() || password.empty())
        return fail("username and password required");

    // 检查用户名是否已存在 - searchUsers 使用精确匹配当搜索词与用户名长度相同时
    // 如果 searchUsers 返回任何结果，说明用户名已存在（因为我们搜索的是完整用户名）
    auto existingUsers = store->searchUsers(username);
    if (!existingUsers.empty()) {
        return fail("username already exists");
    }

    auto userId = store->addUser(username, password);
    auto token = store->createToken(userId);
    return LoginResponse{userId, token};
}

Result<LoginResponse> MockAuthService::login(
    const std::string& username, const std::string& password) {
    auto userId = store->authenticate(username, password);
    if (!userId)
        return fail("invalid username or password");

    auto token = store->createToken(userId);
    return LoginResponse{userId, token};
}

VoidResult MockAuthService::logout(const std::string& token) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    store->removeToken(token);
    return success();
}

Result<core::User> MockAuthService::getCurrentUser(const std::string& token) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto user = store->findUser(userId);
    if (!user)
        return fail("user not found");

    return *user;
}

} // namespace wechat::network
