#include "ChatWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QDateTime>

#include <wechat/core/Message.h>

namespace wechat {
namespace chat {

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent),
      messageListView_(nullptr),
      messageInput_(nullptr),
      sendButton_(nullptr),
      titleLabel_(nullptr) {
    setupUI();
    setupConnections();
}

void ChatWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);

    // 标题栏
    auto *titleBar = new QHBoxLayout();
    titleLabel_ = new QLabel("Chat");
    titleLabel_->setStyleSheet(
        "font-size: 18px; font-weight: bold; margin: 10px;");
    titleBar->addWidget(titleLabel_);
    titleBar->addStretch();
    mainLayout->addLayout(titleBar);

    // 消息列表
    messageListView_ = new MessageListView();
    messageListView_->setStyleSheet("QListWidget {"
                                    "    background-color: #EDEDED;"
                                    "    border: none;"
                                    "}");
    mainLayout->addWidget(messageListView_);

    // 输入区域
    auto *inputArea = new QHBoxLayout();
    messageInput_ = new QLineEdit();
    messageInput_->setPlaceholderText("Type a message...");
    messageInput_->setStyleSheet("QLineEdit {"
                                 "    padding: 10px;"
                                 "    font-size: 14px;"
                                 "    border: 1px solid #ccc;"
                                 "    border-radius: 15px;"
                                 "    margin: 10px;"
                                 "}");

    sendButton_ = new QPushButton("Send");
    sendButton_->setStyleSheet("QPushButton {"
                               "    background-color: #4CAF50;"
                               "    color: white;"
                               "    border: none;"
                               "    padding: 10px 20px;"
                               "    margin: 10px;"
                               "    border-radius: 15px;"
                               "}"
                               "QPushButton:hover {"
                               "    background-color: #45a049;"
                               "}");

    inputArea->addWidget(messageInput_);
    inputArea->addWidget(sendButton_);
    mainLayout->addLayout(inputArea);

    // 设置主窗口属性
    setWindowTitle("Chat Window");
    resize(600, 800);
}

void ChatWidget::setupConnections() {
    connect(sendButton_, &QPushButton::clicked, this, &ChatWidget::sendMessage);

    // 回车发送消息
    connect(messageInput_, &QLineEdit::returnPressed, this,
            &ChatWidget::sendMessage);
}

void ChatWidget::setCurrentUser(core::User const &user) {
    currentUser_ = user;
}

void ChatWidget::setChatPartner(core::User const &partner) {
    chatPartner_ = partner;
    if (titleLabel_) {
        titleLabel_->setText(
            QString::fromStdString("Chat with " + chatPartner_.id));
    }
}

void ChatWidget::sendMessage() {
    QString text = messageInput_->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    // 创建消息对象
    core::Message message;
    message.id = "msg_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
    message.senderId = currentUser_.id;
    // 为演示目的，使用固定的chatId
    message.chatId = "chat_" + currentUser_.id + "_" + chatPartner_.id;
    message.timestamp = QDateTime::currentMSecsSinceEpoch();
    message.updatedAt = 0;
    message.revoked = false;
    message.readCount = 0;
    message.replyTo = "";

    // 设置消息内容
    core::TextContent textContent;
    textContent.text = text.toStdString();
    message.content = {textContent}; // 初始化内容块列表，包含一个文本内容

    // 添加到消息列表
    messageListView_->addMessage(message, currentUser_);

    // 清空输入框
    messageInput_->clear();
}

} // namespace chat
} // namespace wechat
