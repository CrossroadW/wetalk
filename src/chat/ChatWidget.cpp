#include "ChatWidget.h"
#include "ChatController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QDateTime>
#include <QTimer>

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
    titleLabel_ = new QLabel(tr("聊天"));
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

    // 回复指示器（默认隐藏）
    replyIndicator_ = new QWidget();
    auto *replyLayout = new QHBoxLayout(replyIndicator_);
    replyLayout->setContentsMargins(10, 5, 10, 5);
    replyLabel_ = new QLabel();
    replyLabel_->setStyleSheet("color: #666; font-size: 12px;");
    cancelReplyButton_ = new QPushButton("×");
    cancelReplyButton_->setFixedSize(20, 20);
    cancelReplyButton_->setStyleSheet(
        "QPushButton { border: none; color: #999; font-weight: bold; }"
        "QPushButton:hover { color: #333; }");
    replyLayout->addWidget(replyLabel_);
    replyLayout->addStretch();
    replyLayout->addWidget(cancelReplyButton_);
    replyIndicator_->setStyleSheet(
        "background-color: #f0f0f0; border-left: 3px solid #4CAF50;");
    replyIndicator_->hide();
    mainLayout->addWidget(replyIndicator_);

    // 输入区域
    auto *inputArea = new QHBoxLayout();
    messageInput_ = new QLineEdit();
    messageInput_->setPlaceholderText(tr("输入消息..."));
    messageInput_->setStyleSheet("QLineEdit {"
                                 "    padding: 10px;"
                                 "    font-size: 14px;"
                                 "    border: 1px solid #ccc;"
                                 "    border-radius: 15px;"
                                 "    margin: 10px;"
                                 "}");

    sendButton_ = new QPushButton(tr("发送"));
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

    // 右键菜单
    connect(messageListView_, &MessageListView::replyRequested, this,
            &ChatWidget::onReplyRequested);
    connect(messageListView_, &MessageListView::forwardRequested, this,
            &ChatWidget::onForwardRequested);
    connect(messageListView_, &MessageListView::revokeRequested, this,
            &ChatWidget::onRevokeRequested);

    // 取消回复
    connect(cancelReplyButton_, &QPushButton::clicked, this,
            &ChatWidget::cancelReply);
}

void ChatWidget::setCurrentUser(core::User const &user) {
    currentUser_ = user;
}

void ChatWidget::setChatPartner(core::User const &partner) {
    chatPartner_ = partner;
    if (titleLabel_) {
        titleLabel_->setText(
            tr("与 %1 聊天").arg(QString::fromStdString(chatPartner_.id)));
    }
}

void ChatWidget::setController(ChatController *controller) {
    controller_ = controller;
    if (controller_) {
        connect(controller_, &ChatController::messageSent,
                this, &ChatWidget::onMessageSent);
        connect(controller_, &ChatController::messageSendFailed,
                this, &ChatWidget::onMessageSendFailed);
        connect(controller_, &ChatController::messagesReceived,
                this, &ChatWidget::onMessagesReceived);
    }
}

void ChatWidget::sendMessage() {
    QString text = messageInput_->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (controller_) {
        // 委托给 controller → ChatManager → NetworkClient
        // 如果有回复目标，通过 manager 发送带 replyTo 的消息
        if (!replyToMessageId_.empty()) {
            core::TextContent tc;
            tc.text = text.toStdString();
            controller_->manager().sendMessage({tc}, replyToMessageId_);
            cancelReply();
        } else {
            controller_->onSendText(text);
        }
    } else {
        // 无 controller 时的 fallback（向后兼容）
        core::Message message;
        message.id =
            "msg_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
        message.senderId = currentUser_.id;
        message.chatId = "chat_" + currentUser_.id + "_" + chatPartner_.id;
        message.timestamp = QDateTime::currentMSecsSinceEpoch();
        message.updatedAt = 0;
        message.revoked = false;
        message.readCount = 0;
        message.replyTo = replyToMessageId_;

        core::TextContent textContent;
        textContent.text = text.toStdString();
        message.content = {textContent};

        messageListView_->addMessage(message, currentUser_);
        cancelReply();
    }

    messageInput_->clear();
}

// ── Controller 事件回调 ──

void ChatWidget::onMessageSent(QString /*clientTempId*/,
                                core::Message serverMessage) {
    messageListView_->addMessage(serverMessage, currentUser_);
}

void ChatWidget::onMessageSendFailed(QString /*clientTempId*/,
                                      QString reason) {
    // 简单处理：在标题栏短暂提示失败信息
    if (titleLabel_) {
        auto original = titleLabel_->text();
        titleLabel_->setText(tr("发送失败: ") + reason);
        // 3 秒后恢复
        QTimer::singleShot(3000, this, [this, original]() {
            if (titleLabel_) {
                titleLabel_->setText(original);
            }
        });
    }
}

void ChatWidget::onMessagesReceived(QString /*chatId*/,
                                     std::vector<core::Message> messages) {
    for (auto const &msg : messages) {
        messageListView_->addMessage(msg, currentUser_);
    }
}

// ── 右键菜单处理 ──

void ChatWidget::onReplyRequested(core::Message const &message) {
    replyToMessageId_ = message.id;

    // 提取消息预览文本
    QString preview;
    for (auto const &block : message.content) {
        std::visit(
            [&preview](auto const &content) {
                using T = std::decay_t<decltype(content)>;
                if constexpr (std::is_same_v<T, core::TextContent>) {
                    preview = QString::fromStdString(content.text);
                }
            },
            block);
        if (!preview.isEmpty()) {
            break;
        }
    }
    if (preview.length() > 30) {
        preview = preview.left(30) + "...";
    }

    replyLabel_->setText(tr("回复: ") + preview);
    replyIndicator_->show();
    messageInput_->setFocus();
}

void ChatWidget::onForwardRequested(core::Message const & /*message*/) {
    // TODO: 实现转发功能 - 需要选择目标聊天
    if (titleLabel_) {
        auto original = titleLabel_->text();
        titleLabel_->setText(tr("转发功能开发中..."));
        QTimer::singleShot(2000, this, [this, original]() {
            if (titleLabel_) {
                titleLabel_->setText(original);
            }
        });
    }
}

void ChatWidget::onRevokeRequested(core::Message const &message) {
    if (controller_) {
        controller_->manager().revokeMessage(message.id);
        // TODO: 刷新消息显示（撤回后显示"消息已撤回"）
    }
}

void ChatWidget::cancelReply() {
    replyToMessageId_.clear();
    replyIndicator_->hide();
}

} // namespace chat
} // namespace wechat
