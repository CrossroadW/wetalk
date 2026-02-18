#include <wechat/chat/ChatManager.h>
#include <wechat/core/Event.h>

namespace wechat::chat {

ChatManager::ChatManager(network::NetworkClient &client, core::EventBus &bus)
    : client_(client), bus_(bus) {}

// ── 会话 ──

void ChatManager::setSession(std::string const &token,
                              std::string const &userId) {
    token_ = token;
    userId_ = userId;
}

std::string const &ChatManager::currentUserId() const { return userId_; }

// ── 当前聊天 ──

void ChatManager::openChat(std::string const &chatId) {
    activeChatId_ = chatId;
    if (lastSyncTs_.find(chatId) == lastSyncTs_.end()) {
        lastSyncTs_[chatId] = 0;
    }
    // 首次同步：拉取已有消息
    pollMessages();
}

std::string const &ChatManager::activeChatId() const { return activeChatId_; }

// ── 发送 ──

std::string ChatManager::sendTextMessage(std::string const &text) {
    core::TextContent tc;
    tc.text = text;
    return sendMessage({tc});
}

std::string ChatManager::sendMessage(core::MessageContent const &content,
                                     std::string const &replyTo) {
    auto tempId = generateTempId();

    auto result =
        client_.chat().sendMessage(token_, activeChatId_, replyTo, content);

    if (result.ok()) {
        auto &msg = result.value();
        // 推进同步游标，避免下次 poll 重复拉到自己发的消息
        if (msg.timestamp > lastSyncTs_[activeChatId_]) {
            lastSyncTs_[activeChatId_] = msg.timestamp;
        }
        bus_.publish(
            core::MessageSentEvent{tempId, std::move(result.value())});
    } else {
        bus_.publish(core::MessageSendFailedEvent{
            tempId, result.error().code, result.error().message});
    }

    return tempId;
}

// ── 同步 ──

void ChatManager::pollMessages() {
    if (activeChatId_.empty() || token_.empty()) {
        return;
    }

    int64_t sinceTs = lastSyncTs_[activeChatId_];
    auto result =
        client_.chat().syncMessages(token_, activeChatId_, sinceTs, 50);

    if (result.ok() && !result.value().messages.empty()) {
        auto &msgs = result.value().messages;
        // 推进同步游标
        lastSyncTs_[activeChatId_] = msgs.back().timestamp;

        bus_.publish(core::MessagesReceivedEvent{
            activeChatId_, std::move(result.value().messages)});
    }
}

// ── 撤回 / 编辑 ──

void ChatManager::revokeMessage(std::string const &messageId) {
    auto result = client_.chat().revokeMessage(token_, messageId);
    if (result.ok()) {
        bus_.publish(
            core::MessageRevokedEvent{messageId, activeChatId_});
    }
}

void ChatManager::editMessage(std::string const &messageId,
                               core::MessageContent const &newContent) {
    auto result = client_.chat().editMessage(token_, messageId, newContent);
    if (result.ok()) {
        // 重新同步来获取最新消息内容（简单实现）
        pollMessages();
    }
}

// ── 内部 ──

std::string ChatManager::generateTempId() {
    return "tmp_" + std::to_string(++tempIdCounter_);
}

} // namespace wechat::chat
