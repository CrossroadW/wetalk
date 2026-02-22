#include "MockChatService.h"

#include "MockDataStore.h"

#include <algorithm>

namespace wechat::network {

MockChatService::MockChatService(std::shared_ptr<MockDataStore> store)
    : store(std::move(store)) {}

Result<core::Message> MockChatService::sendMessage(
    const std::string& token, int64_t chatId,
    int64_t replyTo, const core::MessageContent& content) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    if (content.empty())
        return fail("empty content");

    // 验证 chatId 对应的群存在且用户是成员
    auto group = store->findGroup(chatId);
    if (!group)
        return fail("chat not found");

    auto& members = group->memberIds;
    if (std::find(members.begin(), members.end(), userId) == members.end())
        return fail("not a member of this chat");

    auto msg = store->addMessage(userId, chatId, replyTo, content);
    Q_EMIT messageStored(chatId);
    return msg;
}

Result<SyncMessagesResponse> MockChatService::fetchAfter(
    const std::string& token, int64_t chatId,
    int64_t afterId, int limit) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto msgs = store->getMessagesAfter(chatId, afterId, limit + 1);
    bool hasMore = static_cast<int>(msgs.size()) > limit;
    if (hasMore) {
        if (afterId == 0) {
            // 取最新 N 条：多出来的是最旧的，从前面删
            msgs.erase(msgs.begin());
        } else {
            // 取 afterId 之后 N 条：多出来的是最新的，从后面删
            msgs.pop_back();
        }
    }

    return SyncMessagesResponse{std::move(msgs), hasMore};
}

Result<SyncMessagesResponse> MockChatService::fetchBefore(
    const std::string& token, int64_t chatId,
    int64_t beforeId, int limit) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto msgs = store->getMessagesBefore(chatId, beforeId, limit + 1);
    bool hasMore = static_cast<int>(msgs.size()) > limit;
    if (hasMore) {
        if (beforeId == 0) {
            // 取最早 N 条：多出来的是最新的，从后面删
            msgs.pop_back();
        } else {
            // 取 beforeId 之前 N 条：多出来的是最旧的，从前面删
            msgs.erase(msgs.begin());
        }
    }

    return SyncMessagesResponse{std::move(msgs), hasMore};
}

Result<SyncMessagesResponse> MockChatService::fetchUpdated(
    const std::string& token, int64_t chatId,
    int64_t startId, int64_t endId,
    int64_t updatedAt, int limit) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto msgs = store->getMessagesUpdatedAfter(chatId, startId, endId, updatedAt, limit + 1);
    bool hasMore = static_cast<int>(msgs.size()) > limit;
    if (hasMore) msgs.pop_back();

    return SyncMessagesResponse{std::move(msgs), hasMore};
}

VoidResult MockChatService::revokeMessage(const std::string& token,
                                          int64_t messageId) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto msg = store->findMessage(messageId);
    if (!msg)
        return fail("message not found");

    if (msg->senderId != userId)
        return fail("can only revoke own messages");

    msg->revoked = true;
    msg->updatedAt = store->now();
    store->saveMessage(*msg);
    Q_EMIT messageUpdated(msg->chatId, messageId);
    return success();
}

VoidResult MockChatService::editMessage(
    const std::string& token, int64_t messageId,
    const core::MessageContent& newContent) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto msg = store->findMessage(messageId);
    if (!msg)
        return fail("message not found");

    if (msg->senderId != userId)
        return fail("can only edit own messages");

    if (msg->revoked)
        return fail("cannot edit revoked message");

    auto ts = store->now();
    msg->content = newContent;
    msg->editedAt = ts;
    msg->updatedAt = ts;
    store->saveMessage(*msg);
    Q_EMIT messageUpdated(msg->chatId, messageId);
    return success();
}

VoidResult MockChatService::markRead(const std::string& token,
                                     int64_t chatId,
                                     int64_t lastMessageId) {
    auto userId = store->resolveToken(token);
    if (!userId)
        return fail("invalid token");

    auto msg = store->findMessage(lastMessageId);
    if (!msg)
        return fail("message not found");

    msg->readCount++;
    msg->updatedAt = store->now();
    store->saveMessage(*msg);
    Q_EMIT messageUpdated(chatId, lastMessageId);
    return success();
}

} // namespace wechat::network
