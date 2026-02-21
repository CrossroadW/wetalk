#include <wechat/chat/ChatPresenter.h>

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

// ── 聊天初始化 ──

void ChatPresenter::openChat(std::string const& chatId) {
    if (cursors_.find(chatId) == cursors_.end()) {
        cursors_[chatId] = SyncCursor{};
    }
}

// ── 操作 ──

void ChatPresenter::sendTextMessage(std::string const& chatId,
                                     std::string const& text) {
    core::TextContent tc;
    tc.text = text;
    sendMessage(chatId, {tc});
}

void ChatPresenter::sendMessage(std::string const& chatId,
                                 core::MessageContent const& content,
                                 int64_t replyTo) {
    client_.chat().sendMessage(token_, chatId, replyTo, content);
}

void ChatPresenter::loadHistory(std::string const& chatId, int limit) {
    if (chatId.empty() || token_.empty()) {
        return;
    }

    auto& cursor = cursors_[chatId];
    int64_t beforeId = cursor.start > 0 ? cursor.start : 0;
    auto result =
        client_.chat().fetchBefore(token_, chatId, beforeId, limit);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.start = msgs.front().id;
        if (cursor.end == 0) {
            cursor.end = msgs.back().id;
        }
        updateMaxUpdatedAt(cursor, msgs);
        Q_EMIT messagesInserted(QString::fromStdString(chatId), msgs);
        syncUpdated(chatId);
    }
}

void ChatPresenter::loadLatest(std::string const& chatId, int limit) {
    if (chatId.empty() || token_.empty()) {
        return;
    }

    auto& cursor = cursors_[chatId];
    // afterId=0 → 返回最新 limit 条
    auto result =
        client_.chat().fetchAfter(token_, chatId, 0, limit);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.start = msgs.front().id;
        cursor.end = msgs.back().id;
        updateMaxUpdatedAt(cursor, msgs);
        Q_EMIT messagesInserted(QString::fromStdString(chatId), msgs);
        syncUpdated(chatId);
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

    auto& cursor = cursors_[chatId];
    auto result =
        client_.chat().fetchAfter(token_, chatId, cursor.end, 50);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.end = msgs.back().id;
        if (cursor.start == 0) {
            cursor.start = msgs.front().id;
        }
        updateMaxUpdatedAt(cursor, msgs);
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
            auto& cursor = cursors_[chatId];
            if (msg.updatedAt > cursor.maxUpdatedAt) {
                cursor.maxUpdatedAt = msg.updatedAt;
            }
            Q_EMIT messageUpdated(QString::fromStdString(chatId), msg);
        }
    }
}

// ── 增量同步 ──

void ChatPresenter::syncUpdated(std::string const& chatId) {
    if (chatId.empty() || token_.empty()) {
        return;
    }

    auto& cursor = cursors_[chatId];
    if (cursor.start == 0 || cursor.end == 0) {
        return;
    }

    auto result = client_.chat().fetchUpdated(
        token_, chatId, cursor.start, cursor.end,
        cursor.maxUpdatedAt, 100);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        updateMaxUpdatedAt(cursor, msgs);
        for (auto const& msg : msgs) {
            Q_EMIT messageUpdated(QString::fromStdString(chatId), msg);
        }
    }
}

void ChatPresenter::updateMaxUpdatedAt(
    SyncCursor& cursor, std::vector<core::Message> const& msgs) {
    for (auto const& msg : msgs) {
        if (msg.updatedAt > cursor.maxUpdatedAt) {
            cursor.maxUpdatedAt = msg.updatedAt;
        }
    }
}

} // namespace wechat::chat
