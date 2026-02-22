#include "ChatSandbox.h"

#include <QVBoxLayout>

#include <spdlog/spdlog.h>

namespace wechat {
namespace chat {

ChatSandbox::ChatSandbox(QWidget* parent) : QWidget(parent) {
    client = network::createMockClient();
    auto reg = client->auth().registerUser("me", "pass");
    myToken = reg->token;
    myUserId = reg->id;

    setupUI();

    chatPage->setSession(myToken, myUserId);
}

void ChatSandbox::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    addButton = new QPushButton(tr("+ New Chat"));
    addButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #07C160; color: white;"
        "    border: none; padding: 10px; font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #06AD56; }");
    layout->addWidget(addButton);

    chatPage = new ChatPage(*client);
    layout->addWidget(chatPage);

    setWindowTitle("WeTalk Sandbox");
    resize(900, 700);

    connect(addButton, &QPushButton::clicked, this, &ChatSandbox::onAddChat);
}

void ChatSandbox::onAddChat() {
    ++peerCounter;
    std::string peerName = "user_" + std::to_string(peerCounter);

    auto reg = client->auth().registerUser(peerName, "pass");
    if (!reg.has_value()) {
        spdlog::warn("Failed to register peer: {}", peerName);
        return;
    }
    auto peerId = reg->id;
    auto peerToken = reg->token;

    // 建立好友 & 创建群聊
    client->contacts().addFriend(myToken, peerId);
    auto group = client->groups().createGroup(myToken, {myUserId, peerId});
    if (!group.has_value()) {
        spdlog::warn("Failed to create group with {}", peerName);
        return;
    }
    auto chatId = group->id;

    // MockBackend 预灌历史消息
    auto* backend = new MockBackend(*client, this);
    backend->setPeerSession(peerToken, peerId);
    backend->setChatId(chatId);

    // 临时清除 session，防止 onMessageStored 自动同步
    chatPage->chatPresenter()->setSession("", 0);
    backend->prefill(100, {myToken, peerToken});
    chatPage->chatPresenter()->setSession(myToken, myUserId);

    spdlog::info("Pre-filled 100 messages in chat {}", chatId);

    // 通过 ChatPage 打开聊天
    core::User peer;
    peer.id = peerId;
    peer.username = peerName;
    chatPage->openChat(chatId, peer);

    // 启动测试脚本
    backend->runScript(MockBackend::typicalScript());
    backends[chatId] = backend;

    spdlog::info("New chat created: {} <-> {} (MockBackend active)",
                 myUserId, peerName);
}

} // namespace chat
} // namespace wechat
