#include "MockContactService.h"

#include "MockDataStore.h"

namespace wechat::network {

MockContactService::MockContactService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

VoidResult MockContactService::addFriend(const std::string& token,
                                         const std::string& targetUserId) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    if (userId == targetUserId)
        return {ErrorCode::InvalidArgument, "cannot add yourself"};

    if (!store->findUser(targetUserId))
        return {ErrorCode::NotFound, "user not found"};

    if (store->areFriends(userId, targetUserId))
        return {ErrorCode::AlreadyExists, "already friends"};

    store->addFriendship(userId, targetUserId);
    return success();
}

VoidResult MockContactService::removeFriend(const std::string& token,
                                            const std::string& targetUserId) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    if (!store->areFriends(userId, targetUserId))
        return {ErrorCode::NotFound, "not friends"};

    store->removeFriendship(userId, targetUserId);
    return success();
}

Result<std::vector<core::User>> MockContactService::listFriends(
    const std::string& token) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto friendIds = store->getFriendIds(userId);
    std::vector<core::User> result;
    for (auto& fid : friendIds) {
        auto* u = store->findUser(fid);
        if (u) result.push_back(*u);
    }
    return result;
}

Result<std::vector<core::User>> MockContactService::searchUser(
    const std::string& token, const std::string& keyword) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    return store->searchUsers(keyword);
}

} // namespace wechat::network
