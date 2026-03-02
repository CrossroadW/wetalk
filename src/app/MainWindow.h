#pragma once

#include <wechat/network/NetworkClient.h>

#include <QWidget>
#include <memory>

namespace wechat {

namespace chat {
class ChatPage;
}

namespace contacts {
class ContactsWidget;
class ContactsPresenter;
}

namespace app {

class IconSidebar;

/// 主窗口（WeChat 风格三栏布局）
class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(network::NetworkClient& client,
                        QWidget* parent = nullptr);

    /// 设置当前用户会话
    void setSession(const std::string& token, int64_t userId);

private:
    void setupUI();

    network::NetworkClient& client;

    IconSidebar* iconSidebar = nullptr;
    chat::ChatPage* chatPage = nullptr;
    contacts::ContactsWidget* contactsWidget = nullptr;
    contacts::ContactsPresenter* contactsPresenter = nullptr;
};

}  // namespace app
}  // namespace wechat
