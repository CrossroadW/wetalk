#include "MockGroupService.h"

#include "MockDataStore.h"

#include <algorithm>

namespace wechat {
namespace network {

MockGroupService::MockGroupService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<core::Group> MockGroupService::createGroup(
    const std::string& token,
    const std::vector<std::string>& memberIds) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    // 确保创建者在成员列表中
    auto ids = memberIds;
    if (std::find(ids.begin(), ids.end(), userId) == ids.end())
        ids.insert(ids.begin(), userId);

    auto& group = store->createGroup(userId, ids);
    return group;
}

VoidResult MockGroupService::dissolveGroup(const std::string& token,
                                           const std::string& groupId) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* group = store->findGroup(groupId);
    if (!group)
        return {ErrorCode::NotFound, "group not found"};

    if (group->ownerId != userId)
        return {ErrorCode::PermissionDenied, "only owner can dissolve"};

    store->removeGroup(groupId);
    return success();
}

VoidResult MockGroupService::addMember(const std::string& token,
                                       const std::string& groupId,
                                       const std::string& userId) {
    auto callerId = store->resolveToken(token);
    if (callerId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* group = store->findGroup(groupId);
    if (!group)
        return {ErrorCode::NotFound, "group not found"};

    if (!store->findUser(userId))
        return {ErrorCode::NotFound, "user not found"};

    auto& m = group->memberIds;
    if (std::find(m.begin(), m.end(), userId) != m.end())
        return {ErrorCode::AlreadyExists, "already a member"};

    m.push_back(userId);
    return success();
}

VoidResult MockGroupService::removeMember(const std::string& token,
                                          const std::string& groupId,
                                          const std::string& userId) {
    auto callerId = store->resolveToken(token);
    if (callerId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* group = store->findGroup(groupId);
    if (!group)
        return {ErrorCode::NotFound, "group not found"};

    if (group->ownerId != callerId)
        return {ErrorCode::PermissionDenied, "only owner can remove members"};

    auto& m = group->memberIds;
    auto it = std::find(m.begin(), m.end(), userId);
    if (it == m.end())
        return {ErrorCode::NotFound, "not a member"};

    m.erase(it);
    return success();
}

Result<std::vector<std::string>> MockGroupService::listMembers(
    const std::string& token, const std::string& groupId) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* group = store->findGroup(groupId);
    if (!group)
        return {ErrorCode::NotFound, "group not found"};

    return group->memberIds;
}

Result<std::vector<core::Group>> MockGroupService::listMyGroups(
    const std::string& token) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    return store->getGroupsByUser(userId);
}

} // namespace network
} // namespace wechat
