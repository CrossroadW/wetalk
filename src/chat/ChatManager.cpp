#include <wechat/chat/ChatManager.h>

namespace wechat::chat {

ChatManager::ChatManager(network::NetworkClient &client,
                         std::shared_ptr<ChatSignals> chatSignals)
    : client_(client), signals_(chatSignals) {}

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
    if (cursors_.find(chatId) == cursors_.end()) {
        cursors_[chatId] = SyncCursor{};
    }
    // 首次同步：拉取最新消息
    pollMessages();
}

std::string const &ChatManager::activeChatId() const { return activeChatId_; }

// ── 发送 ──

int64_t ChatManager::sendTextMessage(std::string const &text) {
    core::TextContent tc;
    tc.text = text;
    return sendMessage({tc});
}

int64_t ChatManager::sendMessage(core::MessageContent const &content,
                                     int64_t replyTo) {
    auto tempId = generateTempId();

    auto result =
        client_.chat().sendMessage(token_, activeChatId_, replyTo, content);

    if (result.ok()) {
        auto &msg = result.value();
        // 推进 end 游标，避免下次 poll 重复拉到自己发的消息
        auto &cursor = cursors_[activeChatId_];
        if (msg.id > cursor.end) {
            cursor.end = msg.id;
        }
        if (cursor.start == 0) {
            cursor.start = msg.id;
        }
        signals_->messageSent(tempId, result.value());
    } else {
        signals_->messageSendFailed(tempId, result.error().code,
                                    result.error().message);
    }

    return tempId;
}

// ── 同步 ──

void ChatManager::pollMessages() {
    if (activeChatId_.empty() || token_.empty()) {
        return;
    }

    auto &cursor = cursors_[activeChatId_];
    auto result =
        client_.chat().fetchAfter(token_, activeChatId_, cursor.end, 50);

    if (result.ok() && !result.value().messages.empty()) {
        auto &msgs = result.value().messages;
        // 推进 end 游标
        cursor.end = msgs.back().id;
        // 如果 start 未初始化，设置为第一条消息的 ID
        if (cursor.start == 0) {
            cursor.start = msgs.front().id;
        }

        signals_->messagesReceived(activeChatId_, result.value().messages);
    }
}

void ChatManager::loadHistory(int limit) {
    if (activeChatId_.empty() || token_.empty()) {
        return;
    }

    auto &cursor = cursors_[activeChatId_];
    // start == 0 表示还没有任何消息，用 INT64_MAX 从最新开始
    int64_t beforeId = cursor.start > 0 ? cursor.start : INT64_MAX;
    auto result =
        client_.chat().fetchBefore(token_, activeChatId_, beforeId, limit);

    if (result.ok() && !result.value().messages.empty()) {
        auto &msgs = result.value().messages;
        // 推进 start 游标
        cursor.start = msgs.front().id;
        // 如果 end 未初始化，设置为最后一条消息的 ID
        if (cursor.end == 0) {
            cursor.end = msgs.back().id;
        }

        signals_->messagesReceived(activeChatId_, result.value().messages);
    }
}

// ── 撤回 / 编辑 ──

void ChatManager::revokeMessage(int64_t messageId) {
    auto result = client_.chat().revokeMessage(token_, messageId);
    if (result.ok()) {
        signals_->messageRevoked(messageId, activeChatId_);
    }
}

void ChatManager::editMessage(int64_t messageId,
                               core::MessageContent const &newContent) {
    auto result = client_.chat().editMessage(token_, messageId, newContent);
    if (result.ok()) {
        // 重新同步来获取最新消息内容（简单实现）
        pollMessages();
    }
}

// ── 内部 ──

int64_t ChatManager::generateTempId() {
    return -(++tempIdCounter_);
}

} // namespace wechat::chat
