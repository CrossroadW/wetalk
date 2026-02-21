#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>

#include <wechat/chat/ChatPresenter.h>
#include <wechat/network/NetworkClient.h>

#include "../ChatWidget.h"

#include <map>
#include <memory>
#include <string>

namespace wechat::chat {

/// 沙盒多聊天容器
///
/// 左侧联系人列表 + 右侧 ChatWidget 栈，模拟微信主界面。
/// 可随意添加用户、创建聊天、切换会话，仅用于开发调试。
class ChatSandbox : public QWidget {
    Q_OBJECT

public:
    explicit ChatSandbox(QWidget* parent = nullptr);

private Q_SLOTS:
    void onAddChat();
    void onContactClicked(QListWidgetItem* item);

private:
    struct ChatEntry {
        std::string chatId;
        std::string peerId;
        std::string peerName;
        ChatWidget* widget = nullptr;
    };

    void setupUI();
    void switchToChat(std::string const& chatId);

    // 网络层 & Presenter（沙盒拥有生命周期）
    std::unique_ptr<network::NetworkClient> client_;
    std::unique_ptr<ChatPresenter> presenter_;

    // 当前用户
    std::string myToken_;
    std::string myUserId_;

    // 聊天列表
    std::map<std::string, ChatEntry> chats_;  // chatId -> entry
    int peerCounter_ = 0;

    // UI
    QListWidget* contactList_ = nullptr;
    QStackedWidget* chatStack_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QWidget* placeholder_ = nullptr;
};

} // namespace wechat::chat
