#include "MockMomentService.h"

#include "MockDataStore.h"

#include <algorithm>
#include <set>

namespace wechat {
namespace network {

MockMomentService::MockMomentService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<Moment> MockMomentService::postMoment(
    const std::string& token, const std::string& text,
    const std::vector<std::string>& imageIds) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    if (text.empty() && imageIds.empty())
        return fail("moment must have text or images");

    auto moment = store->addMoment(userId, text, imageIds);
    return moment;
}

Result<std::vector<Moment>> MockMomentService::listMoments(
    const std::string& token, int64_t beforeTs, int limit) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    // 可见范围：自己 + 好友
    auto friendIds = store->getFriendIds(userId);
    std::set<int64_t> visible(friendIds.begin(), friendIds.end());
    visible.insert(userId);

    return store->getMoments(visible, beforeTs, limit);
}

VoidResult MockMomentService::likeMoment(const std::string& token,
                                         int64_t momentId) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto moment = store->findMoment(momentId);
    if (!moment)
        return fail("moment not found");

    if (store->hasLiked(momentId, userId))
        return fail("already liked");

    store->addLike(momentId, userId);
    return success();
}

Result<Moment::Comment> MockMomentService::commentMoment(
    const std::string& token, int64_t momentId,
    const std::string& text) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto moment = store->findMoment(momentId);
    if (!moment)
        return fail("moment not found");

    if (text.empty())
        return fail("comment text required");

    auto commentId = store->addComment(momentId, userId, text);
    // 重新加载获取完整 comment（含数据库生成的 timestamp）
    auto updated = store->findMoment(momentId);
    if (updated) {
        for (auto const& c : updated->comments) {
            if (c.id == commentId) return c;
        }
    }
    return Moment::Comment{commentId, userId, text, store->now()};
}

} // namespace network
} // namespace wechat
