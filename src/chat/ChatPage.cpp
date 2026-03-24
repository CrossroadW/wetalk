#include "ChatPage.h"

#include "ChatWidget.h"
#include "SessionListWidget.h"

#include <wechat/network/ChatTransport.h>

#include "../network/WsChatTransport.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

namespace wechat {
namespace chat {

ChatPage::ChatPage(network::NetworkClient& client, QWidget* parent)
    : QWidget(parent), client_(client)
{
    // 有真实 WebSocket 连接时才启用 MessageCache 路径
    if (auto* ws = client_.ws()) {
        chatTransport_ = std::make_unique<network::WsChatTransport>(*ws);
        messageCache_  = std::make_unique<cache::MessageCache>(*chatTransport_);
        chatPresenter_ = std::make_unique<ChatPresenter>(*messageCache_);
    }
    // Mock 模式（ws() == nullptr）时 chatPresenter_ 保持 nullptr，
    // ChatWidget::setPresenter(nullptr) 会静默跳过所有网络操作。

    sessionPresenter_ = std::make_unique<SessionPresenter>(client_);
    setupUI();
}

void ChatPage::setSession(const std::string& token, int64_t userId) {
    token_  = token;
    userId_ = userId;

    if (chatPresenter_)
        chatPresenter_->setSession(token, userId);
    sessionPresenter_->setSession(token, userId);

    sessionList_->setPresenter(sessionPresenter_.get());
}

void ChatPage::openChat(int64_t chatId, const core::User& peer) {
    peers_[chatId] = peer;
    auto* widget = getOrCreateChatWidget(chatId);
    chatStack_->setCurrentWidget(widget);

    // 让 SessionPresenter 刷新列表以显示新会话
    sessionPresenter_->loadSessions();
}

void ChatPage::setupUI() {
    // 左侧：会话列表
    sessionList_ = new SessionListWidget;
    sessionList_->setStyleSheet(R"(
        SessionListWidget {
            background-color: #f5f5f5;
            border-right: 1px solid #e0e0e0;
        }
    )");

    // 右侧：聊天栈
    chatStack_ = new QStackedWidget;
    chatStack_->setStyleSheet("QStackedWidget { background-color: #ffffff; }");

    placeholder_ = new QWidget;
    placeholder_->setStyleSheet("QWidget { background-color: #ffffff; }");
    auto* phLayout = new QVBoxLayout(placeholder_);
    auto* phLabel = new QLabel("Select a chat to start");
    phLabel->setAlignment(Qt::AlignCenter);
    phLabel->setStyleSheet("color: #999; font-size: 16px;");
    phLayout->addWidget(phLabel);
    chatStack_->addWidget(placeholder_);

    // Splitter 组合
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #e0e0e0;
            width: 1px;
        }
    )");
    splitter->addWidget(sessionList_);
    splitter->addWidget(chatStack_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({250, 600});

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(splitter);

    connect(sessionList_, &SessionListWidget::sessionSelected,
            this, &ChatPage::onSessionSelected);
}

void ChatPage::onSessionSelected(int64_t chatId) {
    auto* widget = getOrCreateChatWidget(chatId);
    chatStack_->setCurrentWidget(widget);
}

ChatWidget* ChatPage::getOrCreateChatWidget(int64_t chatId) {
    auto it = chatWidgets_.find(chatId);
    if (it != chatWidgets_.end()) {
        return it->second;
    }

    auto* widget = new ChatWidget;
    widget->setCurrentUser(core::User{userId_});
    widget->setChatId(chatId);

    auto peerIt = peers_.find(chatId);
    if (peerIt != peers_.end()) {
        widget->setChatPartner(peerIt->second);
    }

    widget->setPresenter(chatPresenter_.get()); // nullptr-safe
    chatStack_->addWidget(widget);
    chatWidgets_[chatId] = widget;

    return widget;
}

} // namespace chat
} // namespace wechat
