#include <wechat/chat/ChatPresenter.h>

#include <climits>

namespace wechat::chat {

ChatPresenter::ChatPresenter(network::NetworkClient& client, QObject* parent)
    : QObject(parent), client_(client) {
    storedConn_ = client_.chat().onMessageStored.connect(
        [this](const std::string& chatId) {
            onNetworkMessageStored(chatId);
        });

    updatedConn_ = client_.chat().onMessageUpdated.connect(
        [this](const std::string& chatId, int64_t messageId) {
            onNetworkMessageUpdated(chatId, messageId);
        });
}

ChatPresenter::~ChatPresenter() = default;

// ── 会话 ──

void ChatPresenter::setSession(std::string const& token,
                                std::string const& userId) {
    token_ = token;
    userId_ = userId;
}

std::string const& ChatPresenter::currentUserId() const { return userId_; }

// ── 当前聊天 ──

void ChatPresenter::openChat(std::string const& chatId) {
    activeChatId_ = chatId;
    if (cursors_.find(chatId) == cursors_.end()) {
        cursors_[chatId] = SyncCursor{};
    }
    onNetworkMessageStored(chatId);
}

std::string const& ChatPresenter::activeChatId() const {
    return activeChatId_;
}

// ── 操作 ──

void ChatPresenter::sendTextMessage(std::string const& text) {
    core::TextContent tc;
    tc.text = text;
    sendMessage({tc});
}

void ChatPresenter::sendMessage(core::MessageContent const& content,
                                 int64_t replyTo) {
    client_.chat().sendMessage(token_, activeChatId_, replyTo, content);
}

void ChatPresenter::loadHistory(int limit) {
    if (activeChatId_.empty() || token_.empty()) {
        return;
    }

    auto& cursor = cursors_[activeChatId_];
    int64_t beforeId = cursor.start > 0 ? cursor.start : INT64_MAX;
    auto result =
        client_.chat().fetchBefore(token_, activeChatId_, beforeId, limit);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.start = msgs.front().id;
        if (cursor.end == 0) {
            cursor.end = msgs.back().id;
        }
        Q_EMIT messagesInserted(QString::fromStdString(activeChatId_), msgs);
    }
}

void ChatPresenter::revokeMessage(int64_t messageId) {
    client_.chat().revokeMessage(token_, messageId);
}

void ChatPresenter::editMessage(int64_t messageId,
                                 core::MessageContent const& newContent) {
    client_.chat().editMessage(token_, messageId, newContent);
}

// ── 网络层通知回调 ──

void ChatPresenter::onNetworkMessageStored(std::string const& chatId) {
    if (token_.empty()) {
        return;
    }

    // 只处理已打开过的聊天；未打开的聊天由 openChat() 做首次同步
    auto it = cursors_.find(chatId);
    if (it == cursors_.end()) {
        return;
    }
    auto& cursor = it->second;
    auto result =
        client_.chat().fetchAfter(token_, chatId, cursor.end, 50);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.end = msgs.back().id;
        if (cursor.start == 0) {
            cursor.start = msgs.front().id;
        }
        Q_EMIT messagesInserted(QString::fromStdString(chatId), msgs);
    }
}

void ChatPresenter::onNetworkMessageUpdated(std::string const& chatId,
                                              int64_t messageId) {
    if (token_.empty()) {
        return;
    }

    auto result =
        client_.chat().fetchAfter(token_, chatId, messageId - 1, 1);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msg = result.value().messages[0];
        if (msg.id == messageId) {
            Q_EMIT messageUpdated(QString::fromStdString(chatId), msg);
        }
    }
}

} // namespace wechat::chat
