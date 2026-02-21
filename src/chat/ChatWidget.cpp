#include "ChatWidget.h"

#include <wechat/chat/ChatPresenter.h>

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

ChatWidget::ChatWidget(QWidget* parent)
    : QWidget(parent),
      messageListView_(nullptr),
      messageInput_(nullptr),
      sendButton_(nullptr),
      titleLabel_(nullptr) {
    setupUI();
    setupConnections();
}

void ChatWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // 标题栏
    auto* titleBar = new QHBoxLayout();
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

    // toast 提示（覆盖在消息列表顶部，默认隐藏）
    toastLabel_ = new QLabel(messageListView_);
    toastLabel_->setAlignment(Qt::AlignCenter);
    toastLabel_->setStyleSheet(
        "QLabel {"
        "    background-color: rgba(0,0,0,0.6); color: white;"
        "    border-radius: 10px; padding: 6px 16px;"
        "    font-size: 12px;"
        "}");
    toastLabel_->setFixedHeight(30);
    toastLabel_->hide();

    // 回复指示器（默认隐藏）
    replyIndicator_ = new QWidget();
    auto* replyLayout = new QHBoxLayout(replyIndicator_);
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
    auto* inputArea = new QHBoxLayout();
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

    setWindowTitle("Chat Window");
    resize(600, 800);
}

void ChatWidget::setupConnections() {
    connect(sendButton_, &QPushButton::clicked, this, &ChatWidget::sendMessage);
    connect(messageInput_, &QLineEdit::returnPressed, this,
            &ChatWidget::sendMessage);

    // 右键菜单
    connect(messageListView_, &MessageListView::replyRequested, this,
            &ChatWidget::onReplyRequested);
    connect(messageListView_, &MessageListView::forwardRequested, this,
            &ChatWidget::onForwardRequested);
    connect(messageListView_, &MessageListView::revokeRequested, this,
            &ChatWidget::onRevokeRequested);

    // 滚动到顶部 → 加载历史
    connect(messageListView_, &MessageListView::reachedTop, this,
            &ChatWidget::onReachedTop);

    // 取消回复
    connect(cancelReplyButton_, &QPushButton::clicked, this,
            &ChatWidget::cancelReply);
}

void ChatWidget::setCurrentUser(core::User const& user) {
    currentUser_ = user;
}

void ChatWidget::setChatPartner(core::User const& partner) {
    chatPartner_ = partner;
    if (titleLabel_) {
        titleLabel_->setText(
            tr("与 %1 聊天").arg(QString::fromStdString(chatPartner_.id)));
    }
}

void ChatWidget::setChatId(std::string const& chatId) {
    chatId_ = chatId;
}

void ChatWidget::setPresenter(ChatPresenter* presenter) {
    presenter_ = presenter;
    if (presenter_) {
        connect(presenter_, &ChatPresenter::messagesInserted,
                this, &ChatWidget::onMessagesInserted,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
        connect(presenter_, &ChatPresenter::messageUpdated,
                this, &ChatWidget::onMessageUpdated,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
        connect(presenter_, &ChatPresenter::messageRemoved,
                this, &ChatWidget::onMessageRemoved,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    }
    initChat();
}

void ChatWidget::sendMessage() {
    QString text = messageInput_->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (presenter_) {
        if (replyToMessageId_ != 0) {
            core::TextContent tc;
            tc.text = text.toStdString();
            presenter_->sendMessage(chatId_, {tc}, replyToMessageId_);
            cancelReply();
        } else {
            presenter_->sendTextMessage(chatId_, text.toStdString());
        }
    }

    messageInput_->clear();
}

// ── 模型变化回调 ──

void ChatWidget::onMessagesInserted(QString chatId,
                                     std::vector<core::Message> messages) {
    if (!chatId_.empty() && chatId.toStdString() != chatId_) {
        return;
    }
    for (auto const& msg : messages) {
        messageListView_->addMessage(msg, currentUser_);
    }
    loading_ = false;
}

void ChatWidget::onMessageUpdated(QString chatId,
                                   core::Message message) {
    if (!chatId_.empty() && chatId.toStdString() != chatId_) {
        return;
    }
    // upsert 语义：addMessage 会更新已有消息
    messageListView_->addMessage(message, currentUser_);
}

void ChatWidget::onMessageRemoved(QString chatId, int64_t messageId) {
    if (!chatId_.empty() && chatId.toStdString() != chatId_) {
        return;
    }
    // TODO: 从列表中移除对应消息项
    Q_UNUSED(messageId);
}

// ── 初始化 & 懒加载 ──

void ChatWidget::initChat() {
    if (!presenter_ || chatId_.empty() || initialized_) {
        return;
    }
    initialized_ = true;
    presenter_->openChat(chatId_);
    presenter_->loadHistory(chatId_, 20);
}

void ChatWidget::onReachedTop() {
    if (!presenter_ || chatId_.empty() || loading_) {
        return;
    }
    loading_ = true;

    // 显示 toast
    if (toastLabel_) {
        toastLabel_->setText(tr("加载历史消息..."));
        toastLabel_->adjustSize();
        // 居中于消息列表顶部
        int x = (messageListView_->width() - toastLabel_->width()) / 2;
        toastLabel_->move(x, 8);
        toastLabel_->show();
        QTimer::singleShot(1500, this, [this]() {
            if (toastLabel_) {
                toastLabel_->hide();
            }
        });
    }

    presenter_->loadHistory(chatId_, 20);
}

// ── 右键菜单处理 ──

void ChatWidget::onReplyRequested(core::Message const& message) {
    replyToMessageId_ = message.id;

    QString preview;
    for (auto const& block : message.content) {
        std::visit(
            [&preview](auto const& content) {
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

void ChatWidget::onForwardRequested(core::Message const& /*message*/) {
    // TODO: 实现转发功能
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

void ChatWidget::onRevokeRequested(core::Message const& message) {
    if (presenter_) {
        presenter_->revokeMessage(message.id);
    }
}

void ChatWidget::cancelReply() {
    replyToMessageId_ = 0;
    replyIndicator_->hide();
}

} // namespace chat
} // namespace wechat
