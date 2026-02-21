#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include <wechat/core/Message.h>

#include "MessageListView.h"

#include <vector>

namespace wechat {
namespace chat {

class ChatController;

/**
 * @brief 主聊天界面
 */
class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);

    void setCurrentUser(core::User const &user);
    void setChatPartner(core::User const &partner);

    /// 注入 ChatController，ChatWidget 不拥有其生命周期
    void setController(ChatController *controller);

    // 提供对消息列表的访问，以便在沙盒中添加示例消息
    MessageListView *getMessageListView() {
        return messageListView_;
    }

    void sendMessage();
private Q_SLOTS:


    void onMessageSent(int64_t clientTempId, core::Message serverMessage);
    void onMessageSendFailed(int64_t clientTempId, QString reason);
    void onMessagesReceived(QString chatId,
                            std::vector<core::Message> messages);

    // 右键菜单
    void onReplyRequested(core::Message const &message);
    void onForwardRequested(core::Message const &message);
    void onRevokeRequested(core::Message const &message);

    // 取消回复
    void cancelReply();

private:
    void setupUI();
    void setupConnections();

    MessageListView *messageListView_;
    QLineEdit *messageInput_;
    QPushButton *sendButton_;
    QLabel *titleLabel_;

    core::User currentUser_;
    core::User chatPartner_;

    ChatController *controller_ = nullptr;

    // 回复状态
    int64_t replyToMessageId_ = 0;
    QWidget *replyIndicator_ = nullptr;
    QLabel *replyLabel_ = nullptr;
    QPushButton *cancelReplyButton_ = nullptr;
};

} // namespace chat
} // namespace wechat
