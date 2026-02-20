#include "ChatController.h"

namespace wechat::chat {

ChatController::ChatController(ChatManager &manager,
                               std::shared_ptr<ChatSignals> chatSignals,
                               QObject *parent)
    : QObject(parent), manager_(manager), signals_(chatSignals) {
    // 订阅各个信号
    messageSentConnection_ = signals_->messageSent.connect(
        [this](const std::string& clientTempId, const core::Message& msg) {
            Q_EMIT messageSent(QString::fromStdString(clientTempId), msg);
        });

    messageSendFailedConnection_ = signals_->messageSendFailed.connect(
        [this](const std::string& clientTempId, auto code,
               const std::string& reason) {
            Q_EMIT messageSendFailed(QString::fromStdString(clientTempId),
                                     QString::fromStdString(reason));
        });

    messagesReceivedConnection_ = signals_->messagesReceived.connect(
        [this](const std::string& chatId,
               const std::vector<core::Message>& messages) {
            Q_EMIT messagesReceived(QString::fromStdString(chatId), messages);
        });

    messageRevokedConnection_ = signals_->messageRevoked.connect(
        [this](const std::string& messageId, const std::string& chatId) {
            Q_EMIT messageRevoked(QString::fromStdString(messageId));
        });

    messageEditedConnection_ = signals_->messageEdited.connect(
        [this](const std::string& messageId, const std::string& chatId,
               const core::Message& updatedMessage) {
            Q_EMIT messageEdited(QString::fromStdString(messageId),
                                 updatedMessage);
        });

    connect(&pollTimer_, &QTimer::timeout, this,
            [this]() { manager_.pollMessages(); });
}

ChatController::~ChatController() = default;

ChatManager &ChatController::manager() { return manager_; }

void ChatController::startPolling(int intervalMs) {
    pollTimer_.start(intervalMs);
}

void ChatController::stopPolling() { pollTimer_.stop(); }

// ── slots ──

void ChatController::onSendText(QString const &text) {
    manager_.sendTextMessage(text.toStdString());
}

void ChatController::onOpenChat(QString const &chatId) {
    manager_.openChat(chatId.toStdString());
}

} // namespace wechat::chat
