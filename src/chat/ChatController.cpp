#include "ChatController.h"

#include <wechat/core/Event.h>

#include <variant>

namespace wechat::chat {

ChatController::ChatController(ChatManager &manager, core::EventBus &bus,
                               QObject *parent)
    : QObject(parent), manager_(manager), bus_(bus) {
    // 订阅 EventBus，mock 服务是同步的，回调在主线程执行
    busConnection_ = bus_.subscribe(
        [this](core::Event const &e) { onEvent(e); });

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

// ── EventBus → Qt signals ──

void ChatController::onEvent(core::Event const &event) {
    std::visit(
        [this](auto const &ev) {
            using T = std::decay_t<decltype(ev)>;

            if constexpr (std::is_same_v<T, core::MessageSentEvent>) {
                emit messageSent(
                    QString::fromStdString(ev.clientTempId),
                    ev.serverMessage);
            } else if constexpr (std::is_same_v<T,
                                                core::MessageSendFailedEvent>) {
                emit messageSendFailed(
                    QString::fromStdString(ev.clientTempId),
                    QString::fromStdString(ev.reason));
            } else if constexpr (std::is_same_v<T,
                                                core::MessagesReceivedEvent>) {
                emit messagesReceived(
                    QString::fromStdString(ev.chatId), ev.messages);
            } else if constexpr (std::is_same_v<T,
                                                core::MessageRevokedEvent>) {
                emit messageRevoked(
                    QString::fromStdString(ev.messageId));
            } else if constexpr (std::is_same_v<T,
                                                core::MessageEditedEvent>) {
                emit messageEdited(
                    QString::fromStdString(ev.messageId),
                    ev.updatedMessage);
            }
        },
        event);
}

} // namespace wechat::chat
