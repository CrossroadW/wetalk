#pragma once

#include <QWidget>
#include <QPushButton>

#include <wechat/network/NetworkClient.h>

#include "../ChatPage.h"
#include "../MockBackend.h"

#include <map>
#include <memory>
#include <string>

namespace wechat {
namespace chat {

/// 沙盒容器
///
/// 使用 ChatPage 组合 SessionList + ChatWidget，
/// 添加"+ 新建聊天"按钮预灌数据并启动 MockBackend 脚本。
class ChatSandbox : public QWidget {
    Q_OBJECT

public:
    explicit ChatSandbox(QWidget* parent = nullptr);

private Q_SLOTS:
    void onAddChat();

private:
    void setupUI();

    std::unique_ptr<network::NetworkClient> client;
    ChatPage* chatPage = nullptr;
    QPushButton* addButton = nullptr;

    std::string myToken;
    int64_t myUserId = 0;
    int peerCounter = 0;

    // 保持 MockBackend 存活
    std::map<int64_t, MockBackend*> backends;
};

} // namespace chat
} // namespace wechat
