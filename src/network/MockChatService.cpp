#include "MockChatService.h"

#include "MockDataStore.h"

#include <algorithm>

namespace wechat::network {

MockChatService::MockChatService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<core::Message> MockChatService::sendMessage(
    const std::string& token, const std::string& chatId,
    int64_t replyTo, const core::MessageContent& content) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    if (content.empty())
        return {ErrorCode::InvalidArgument, "empty content"};

    // 验证 chatId 对应的群存在且用户是成员
    auto* group = store->findGroup(chatId);
    if (!group)
        return {ErrorCode::NotFound, "chat not found"};

    auto& members = group->memberIds;
    if (std::find(members.begin(), members.end(), userId) == members.end())
        return {ErrorCode::PermissionDenied, "not a member of this chat"};

    auto& msg = store->addMessage(userId, chatId, replyTo, content);
    onMessageStored(chatId);
    return msg;
}

Result<SyncMessagesResponse> MockChatService::fetchAfter(
    const std::string& token, const std::string& chatId,
    int64_t afterId, int limit) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto msgs = store->getMessagesAfter(chatId, afterId, limit + 1);
    bool hasMore = static_cast<int>(msgs.size()) > limit;
    if (hasMore) msgs.pop_back();

    return SyncMessagesResponse{std::move(msgs), hasMore};
}

Result<SyncMessagesResponse> MockChatService::fetchBefore(
    const std::string& token, const std::string& chatId,
    int64_t beforeId, int limit) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto msgs = store->getMessagesBefore(chatId, beforeId, limit + 1);
    bool hasMore = static_cast<int>(msgs.size()) > limit;
    if (hasMore) msgs.erase(msgs.begin());

    return SyncMessagesResponse{std::move(msgs), hasMore};
}

VoidResult MockChatService::revokeMessage(const std::string& token,
                                          int64_t messageId) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* msg = store->findMessage(messageId);
    if (!msg)
        return {ErrorCode::NotFound, "message not found"};

    if (msg->senderId != userId)
        return {ErrorCode::PermissionDenied, "can only revoke own messages"};

    msg->revoked = true;
    msg->updatedAt = store->now();
    onMessageUpdated(msg->chatId, messageId);
    return success();
}

VoidResult MockChatService::editMessage(
    const std::string& token, int64_t messageId,
    const core::MessageContent& newContent) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* msg = store->findMessage(messageId);
    if (!msg)
        return {ErrorCode::NotFound, "message not found"};

    if (msg->senderId != userId)
        return {ErrorCode::PermissionDenied, "can only edit own messages"};

    if (msg->revoked)
        return {ErrorCode::InvalidArgument, "cannot edit revoked message"};

    auto ts = store->now();
    msg->content = newContent;
    msg->editedAt = ts;
    msg->updatedAt = ts;
    onMessageUpdated(msg->chatId, messageId);
    return success();
}

VoidResult MockChatService::markRead(const std::string& token,
                                     const std::string& chatId,
                                     int64_t lastMessageId) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto* msg = store->findMessage(lastMessageId);
    if (!msg)
        return {ErrorCode::NotFound, "message not found"};

    msg->readCount++;
    msg->updatedAt = store->now();
    onMessageUpdated(chatId, lastMessageId);
    return success();
}

} // namespace wechat::network
