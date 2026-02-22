#include "MockAuthService.h"

#include "MockDataStore.h"

namespace wechat::network {

MockAuthService::MockAuthService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<core::User> MockAuthService::registerUser(
    const std::string& username, const std::string& password) {
    if (username.empty() || password.empty())
        return fail("username and password required");

    // UNIQUE 约束：用户名不能重复
    for (auto& u : store->searchUsers(username)) {
        if (u.username == username)
            return fail("username already exists");
    }

    auto userId = store->addUser(username, password);
    store->createToken(userId);
    return *store->findUser(userId);
}

Result<core::User> MockAuthService::login(
    const std::string& username, const std::string& password) {
    auto userId = store->authenticate(username, password);
    if (!userId)
        return fail("invalid username or password");

    store->createToken(userId);
    return *store->findUser(userId);
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
