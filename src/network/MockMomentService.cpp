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
        return {ErrorCode::Unauthorized, "invalid token"};

    if (text.empty() && imageIds.empty())
        return {ErrorCode::InvalidArgument, "moment must have text or images"};

    auto& moment = store->addMoment(userId, text, imageIds);
    return moment;
}

Result<std::vector<Moment>> MockMomentService::listMoments(
    const std::string& token, int64_t beforeTs, int limit) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return {ErrorCode::Unauthorized, "invalid token"};

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
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* moment = store->findMoment(momentId);
    if (!moment)
        return {ErrorCode::NotFound, "moment not found"};

    auto& likes = moment->likedBy;
    if (std::find(likes.begin(), likes.end(), userId) != likes.end())
        return {ErrorCode::AlreadyExists, "already liked"};

    likes.push_back(userId);
    return success();
}

Result<Moment::Comment> MockMomentService::commentMoment(
    const std::string& token, int64_t momentId,
    const std::string& text) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* moment = store->findMoment(momentId);
    if (!moment)
        return {ErrorCode::NotFound, "moment not found"};

    if (text.empty())
        return {ErrorCode::InvalidArgument, "comment text required"};

    static int64_t commentCounter = 0;
    auto commentId = ++commentCounter;
    auto ts = store->now();
    Moment::Comment comment{commentId, userId, text, ts};
    moment->comments.push_back(comment);
    return comment;
}

} // namespace network
} // namespace wechat
