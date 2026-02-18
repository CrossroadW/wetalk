#pragma once

#include <QObject>
#include <QTimer>
#include <boost/signals2/connection.hpp>

#include <wechat/chat/ChatManager.h>
#include <wechat/core/EventBus.h>
#include <wechat/core/Message.h>

#include <string>
#include <vector>

namespace wechat::chat {

/// Qt 桥接层：订阅 EventBus 事件，转为 Qt signals 供 UI 使用
///
/// 同时持有 QTimer 驱动 ChatManager::pollMessages() 轮询
class ChatController : public QObject {
    Q_OBJECT

public:
    explicit ChatController(ChatManager &manager, core::EventBus &bus,
                            QObject *parent = nullptr);
    ~ChatController() override;

    ChatManager &manager();

    void startPolling(int intervalMs = 2000);
    void stopPolling();

Q_SIGNALS:
    /// 消息发送成功，UI 应显示服务端确认的消息
    void messageSent(QString clientTempId, core::Message serverMessage);

    /// 消息发送失败
    void messageSendFailed(QString clientTempId, QString reason);

    /// 收到新消息（来自轮询同步）
    void messagesReceived(QString chatId,
                          std::vector<core::Message> messages);

    /// 消息被撤回
    void messageRevoked(QString messageId);

    /// 消息被编辑
    void messageEdited(QString messageId, core::Message updatedMessage);

public slots:
    void onSendText(QString const &text);
    void onOpenChat(QString const &chatId);

private:
    void onEvent(core::Event const &event);

    ChatManager &manager_;
    core::EventBus &bus_;
    boost::signals2::scoped_connection busConnection_;
    QTimer pollTimer_;
};

} // namespace wechat::chat
