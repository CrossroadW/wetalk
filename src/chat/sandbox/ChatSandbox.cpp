#include "ChatSandbox.h"

#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

#include <wechat/core/Message.h>

#include <spdlog/spdlog.h>

#include <array>

namespace wechat::chat {

static constexpr std::array kSampleMessages = {
    "你好呀 👋", "今天天气不错", "在忙什么呢？", "哈哈哈 😂",
    "好的，收到", "等一下，我看看", "这个问题我想想", "没问题！",
    "晚点再聊", "刚吃完饭 🍜", "周末有空吗？", "发个红包来 🧧",
    "收到收到 ✅", "了解了", "明天见！", "这也太搞笑了吧",
    "我觉得可以", "再说吧", "好久不见啊", "最近怎么样？",
};

ChatSandbox::ChatSandbox(QWidget* parent) : QWidget(parent) {
    // 1. 创建 Mock 网络客户端 & 注册当前用户
    client_ = network::createMockClient();
    auto reg = client_->auth().registerUser("me", "pass");
    myToken_ = reg.value().token;
    myUserId_ = reg.value().userId;

    // 2. 创建 Presenter
    presenter_ = std::make_unique<ChatPresenter>(*client_);
    presenter_->setSession(myToken_, myUserId_);

    setupUI();
}

void ChatSandbox::setupUI() {
    // 左侧面板：添加按钮 + 联系人列表
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    addButton_ = new QPushButton(tr("+ 新建聊天"));
    addButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #07C160; color: white;"
        "    border: none; padding: 10px; font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #06AD56; }");
    leftLayout->addWidget(addButton_);

    contactList_ = new QListWidget();
    contactList_->setStyleSheet(
        "QListWidget { background: #2E2E2E; color: white; border: none; "
        "font-size: 14px; }"
        "QListWidget::item { padding: 12px 10px; }"
        "QListWidget::item:selected { background: #3A3A3A; }");
    leftLayout->addWidget(contactList_);

    // 右侧：聊天栈
    chatStack_ = new QStackedWidget();
    placeholder_ = new QWidget();
    auto* phLayout = new QVBoxLayout(placeholder_);
    auto* phLabel = new QLabel(tr("点击左侧联系人开始聊天\n或点击「+ 新建聊天」添加"));
    phLabel->setAlignment(Qt::AlignCenter);
    phLabel->setStyleSheet("color: #999; font-size: 16px;");
    phLayout->addWidget(phLabel);
    chatStack_->addWidget(placeholder_);

    // Splitter 组合
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(chatStack_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 580});

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(splitter);

    setWindowTitle("WeTalk Sandbox");
    resize(900, 700);

    // 信号
    connect(addButton_, &QPushButton::clicked, this, &ChatSandbox::onAddChat);
    connect(contactList_, &QListWidget::itemClicked,
            this, &ChatSandbox::onContactClicked);
}

void ChatSandbox::onAddChat() {
    // 自动生成对方用户
    ++peerCounter_;
    std::string peerName = "user_" + std::to_string(peerCounter_);

    auto reg = client_->auth().registerUser(peerName, "pass");
    if (!reg.ok()) {
        spdlog::warn("Failed to register peer: {}", peerName);
        return;
    }
    auto peerId = reg.value().userId;
    auto peerToken = reg.value().token;

    // 建立好友 & 创建群聊
    client_->contacts().addFriend(myToken_, peerId);
    auto group = client_->groups().createGroup(
        myToken_, {myUserId_, peerId});
    if (!group.ok()) {
        spdlog::warn("Failed to create group with {}", peerName);
        return;
    }
    auto chatId = group.value().id;

    // ── 预灌 100 条历史消息 ──
    // 临时清除 Presenter session，防止 onMessageStored 自动同步 cursor
    // 灌完后恢复，这样 initChat → loadHistory 才能按需分页加载
    presenter_->setSession("", "");

    for (int i = 0; i < 100; ++i) {
        auto const& tmpl = kSampleMessages[i % kSampleMessages.size()];
        std::string text = "[" + std::to_string(i + 1) + "] " + tmpl;

        core::TextContent tc;
        tc.text = text;

        // 交替使用双方 token 发送，模拟真实对话
        auto const& token = (i % 3 == 0) ? myToken_ : peerToken;
        client_->chat().sendMessage(token, chatId, 0, {tc});
    }

    // 恢复 session
    presenter_->setSession(myToken_, myUserId_);

    spdlog::info("Pre-filled 100 messages in chat {}", chatId);

    // 创建 ChatWidget（setPresenter 会触发 initChat → loadHistory(20)）
    auto* widget = new ChatWidget();
    widget->setCurrentUser(core::User{myUserId_});
    widget->setChatPartner(core::User{peerId});
    widget->setChatId(chatId);
    widget->setPresenter(presenter_.get());
    chatStack_->addWidget(widget);

    // 记录
    ChatEntry entry;
    entry.chatId = chatId;
    entry.peerId = peerId;
    entry.peerName = peerName;
    entry.widget = widget;
    chats_[chatId] = std::move(entry);

    // 添加到联系人列表
    auto* item = new QListWidgetItem(QString::fromStdString(peerName));
    item->setData(Qt::UserRole, QString::fromStdString(chatId));
    contactList_->addItem(item);

    // 自动切换到新聊天
    contactList_->setCurrentItem(item);
    switchToChat(chatId);

    spdlog::info("New chat created: {} <-> {} (100 messages pre-filled)",
                 myUserId_, peerName);
}

void ChatSandbox::onContactClicked(QListWidgetItem* item) {
    auto chatId = item->data(Qt::UserRole).toString().toStdString();
    switchToChat(chatId);
}

void ChatSandbox::switchToChat(std::string const& chatId) {
    auto it = chats_.find(chatId);
    if (it == chats_.end()) {
        return;
    }
    chatStack_->setCurrentWidget(it->second.widget);
}

} // namespace wechat::chat
