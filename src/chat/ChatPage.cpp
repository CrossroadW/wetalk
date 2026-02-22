#include "ChatPage.h"

#include "ChatWidget.h"
#include "SessionListWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace wechat {
namespace chat {

ChatPage::ChatPage(network::NetworkClient& client, QWidget* parent)
    : QWidget(parent), client(client) {
    chatPresenter_ = std::make_unique<ChatPresenter>(client);
    sessionPresenter_ = std::make_unique<SessionPresenter>(client);
    setupUI();
}

void ChatPage::setSession(const std::string& token, int64_t userId) {
    this->token = token;
    this->userId = userId;

    chatPresenter_->setSession(token, userId);
    sessionPresenter_->setSession(token, userId);

    sessionList->setPresenter(sessionPresenter_.get());
}

void ChatPage::openChat(int64_t chatId, const core::User& peer) {
    peers[chatId] = peer;
    auto* widget = getOrCreateChatWidget(chatId);
    chatStack->setCurrentWidget(widget);

    // 让 SessionPresenter 刷新列表以显示新会话
    sessionPresenter_->loadSessions();
}

void ChatPage::setupUI() {
    // 左侧：会话列表
    sessionList = new SessionListWidget;

    // 右侧：聊天栈
    chatStack = new QStackedWidget;
    placeholder = new QWidget;
    auto* phLayout = new QVBoxLayout(placeholder);
    auto* phLabel = new QLabel("Select a chat to start");
    phLabel->setAlignment(Qt::AlignCenter);
    phLabel->setStyleSheet("color: #999; font-size: 16px;");
    phLayout->addWidget(phLabel);
    chatStack->addWidget(placeholder);

    // Splitter 组合
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(sessionList);
    splitter->addWidget(chatStack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 580});

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(splitter);

    // 信号
    connect(sessionList, &SessionListWidget::sessionSelected,
            this, &ChatPage::onSessionSelected);
}

void ChatPage::onSessionSelected(int64_t chatId) {
    auto* widget = getOrCreateChatWidget(chatId);
    chatStack->setCurrentWidget(widget);
}

ChatWidget* ChatPage::getOrCreateChatWidget(int64_t chatId) {
    auto it = chatWidgets.find(chatId);
    if (it != chatWidgets.end()) {
        return it->second;
    }

    auto* widget = new ChatWidget;
    widget->setCurrentUser(core::User{userId});
    widget->setChatId(chatId);

    // 设置聊天对象名字
    auto peerIt = peers.find(chatId);
    if (peerIt != peers.end()) {
        widget->setChatPartner(peerIt->second);
    }

    widget->setPresenter(chatPresenter_.get());
    chatStack->addWidget(widget);
    chatWidgets[chatId] = widget;

    return widget;
}

} // namespace chat
} // namespace wechat
