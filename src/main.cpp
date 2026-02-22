#include <QApplication>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWidget>

#include <spdlog/spdlog.h>

#include <wechat/contacts/ContactsPresenter.h>
#include <wechat/core/AppPaths.h>
#include <wechat/core/User.h>
#include <wechat/log/Log.h>
#include <wechat/login/LoginPresenter.h>
#include <wechat/network/NetworkClient.h>

// 私有头文件（非跨模块导出）
#include "chat/ChatPage.h"
#include "contacts/ContactsWidget.h"
#include "login/LoginWidget.h"

int main(int argc, char* argv[]) {
    wechat::log::init();
    wechat::core::AppPaths::setDataDir(PROJECT_ROOT_PATH);
    QApplication app(argc, argv);

    // 网络层（Mock）
    auto client = wechat::network::createMockClient();

    // 预注册测试用户
    client->auth().registerUser("alice", "123");
    client->auth().registerUser("bob", "123");
    client->auth().registerUser("carol", "123");

    // ── Login ──
    wechat::login::LoginPresenter loginPresenter(*client);
    wechat::login::LoginWidget loginWidget;
    loginWidget.setPresenter(&loginPresenter);
    loginWidget.setWindowTitle("WeTalk - Login");

    // ── Main Window（登录成功后显示）──
    auto* mainWindow = new QWidget;
    mainWindow->setWindowTitle("WeTalk");
    mainWindow->resize(900, 700);

    auto* mainLayout = new QHBoxLayout(mainWindow);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧 TabBar（垂直）
    auto* tabBar = new QTabBar;
    tabBar->setShape(QTabBar::RoundedWest);
    tabBar->addTab("Chat");
    tabBar->addTab("Contacts");
    mainLayout->addWidget(tabBar);

    // 右侧内容区
    auto* contentStack = new QStackedWidget;
    mainLayout->addWidget(contentStack, 1);

    // ChatPage
    auto* chatPage = new wechat::chat::ChatPage(*client);
    contentStack->addWidget(chatPage);

    // ContactsWidget
    auto* contactsWidget = new wechat::contacts::ContactsWidget;
    contentStack->addWidget(contactsWidget);

    // ContactsPresenter
    auto* contactsPresenter =
        new wechat::contacts::ContactsPresenter(*client, mainWindow);

    // Tab 切换
    QObject::connect(tabBar, &QTabBar::currentChanged,
                     contentStack, &QStackedWidget::setCurrentIndex);

    // 从通讯录选择好友 → 创建群聊 → 跳转到聊天
    QObject::connect(
        contactsWidget, &wechat::contacts::ContactsWidget::friendSelected,
        [chatPage, client = client.get(), tabBar](wechat::core::User peer) {
            // 切换到聊天 Tab
            tabBar->setCurrentIndex(0);

            // 创建或打开与该好友的聊天
            // 查找是否已有群组
            auto groups = client->groups().listMyGroups(
                peer.token.empty() ? "" : peer.token);

            // 简化：直接打开聊天（ChatPage 会处理）
            // 实际应用中需要根据已有群组查找
            chatPage->openChat(peer.id, peer);
        });

    // ── 登录成功 → 切换到主窗口 ──
    QObject::connect(
        &loginWidget, &wechat::login::LoginWidget::loggedIn,
        [mainWindow, chatPage, contactsWidget, contactsPresenter,
         client = client.get()](wechat::core::User user) {
            spdlog::info("Logged in as {} (id={})", user.username, user.id);

            // 设置各模块的 session
            chatPage->setSession(user.token, user.id);

            contactsPresenter->setSession(user.token, user.id);
            contactsWidget->setPresenter(contactsPresenter);

            mainWindow->show();
        });

    // 登录窗口关闭时也关闭主窗口
    QObject::connect(&loginWidget, &QWidget::destroyed,
                     mainWindow, &QWidget::close);

    loginWidget.show();

    spdlog::info("WeTalk started");
    return app.exec();
}
