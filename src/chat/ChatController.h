#pragma once

#include <QObject>
#include <QTimer>
#include <boost/signals2/connection.hpp>

#include <wechat/chat/ChatManager.h>
#include <wechat/chat/ChatSignals.h>
#include <wechat/core/Message.h>

#include <memory>
#include <string>
#include <vector>

namespace wechat::chat {

/// Qt 桥接层：订阅 ChatSignals 事件，转为 Qt signals 供 UI 使用
///
/// 同时持有 QTimer 驱动 ChatManager::pollMessages() 轮询
class ChatController : public QObject {
    Q_OBJECT

public:
    explicit ChatController(ChatManager &manager,
                            std::shared_ptr<ChatSignals> chatSignals,
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

public Q_SLOTS:
    void onSendText(QString const &text);
    void onOpenChat(QString const &chatId);

private:
    ChatManager &manager_;
    std::shared_ptr<ChatSignals> signals_;
    boost::signals2::scoped_connection messageSentConnection_;
    boost::signals2::scoped_connection messageSendFailedConnection_;
    boost::signals2::scoped_connection messagesReceivedConnection_;
    boost::signals2::scoped_connection messageRevokedConnection_;
    boost::signals2::scoped_connection messageEditedConnection_;
    QTimer pollTimer_;
};

} // namespace wechat::chat
