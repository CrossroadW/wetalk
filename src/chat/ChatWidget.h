#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "MessageListView.h"

namespace wechat {
namespace chat {

/**
 * @brief 主聊天界面
 */
class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);

    void setCurrentUser(core::User const &user);
    void setChatPartner(core::User const &partner);

    // 提供对消息列表的访问，以便在沙盒中添加示例消息
    MessageListView *getMessageListView() {
        return messageListView_;
    }

public slots:
    void sendMessage();

private:
    void setupUI();
    void setupConnections();

    MessageListView *messageListView_;
    QLineEdit *messageInput_;
    QPushButton *sendButton_;
    QLabel *titleLabel_;

    core::User currentUser_;
    core::User chatPartner_;
};

} // namespace chat
} // namespace wechat
