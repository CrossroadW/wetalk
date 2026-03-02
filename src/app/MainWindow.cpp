#include "MainWindow.h"

#include "IconSidebar.h"

#include <wechat/contacts/ContactsPresenter.h>
#include <wechat/core/User.h>

#include "../chat/ChatPage.h"
#include "../contacts/ContactsWidget.h"

#include <QHBoxLayout>
#include <QStackedWidget>

#include <spdlog/spdlog.h>

namespace wechat {
namespace app {

MainWindow::MainWindow(network::NetworkClient& client, QWidget* parent)
    : QWidget(parent), client(client) {
    setupUI();
}

void MainWindow::setupUI() {
    setWindowTitle("WeTalk");
    resize(1100, 750);
    setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            background-color: #ffffff;
        }
    )");

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧图标栏
    iconSidebar = new IconSidebar;
    mainLayout->addWidget(iconSidebar);

    // 右侧内容区（三栏布局的中间+右侧）
    auto* contentStack = new QStackedWidget;
    contentStack->setStyleSheet("QStackedWidget { background-color: #f5f5f5; }");
    mainLayout->addWidget(contentStack, 1);

    // ChatPage（已经是左右分栏：SessionList + ChatWidget）
    chatPage = new chat::ChatPage(client);
    contentStack->addWidget(chatPage);

    // ContactsWidget
    contactsWidget = new contacts::ContactsWidget;
    contentStack->addWidget(contactsWidget);

    // ContactsPresenter
    contactsPresenter = new contacts::ContactsPresenter(client, this);

    // 图标栏切换
    connect(iconSidebar, &IconSidebar::tabChanged, contentStack,
            &QStackedWidget::setCurrentIndex);

    // 从通讯录选择好友 → 创建群聊 → 跳转到聊天
    connect(contactsWidget, &contacts::ContactsWidget::friendSelected,
            [this](core::User peer) {
                // 切换到聊天 Tab
                iconSidebar->findChild<QPushButton*>()->click();

                // 创建或打开与该好友的聊天
                auto groups = client.groups().listMyGroups(
                    peer.token.empty() ? "" : peer.token);

                chatPage->openChat(peer.id, peer);
            });
}

void MainWindow::setSession(const std::string& token, int64_t userId) {
    spdlog::info("MainWindow: setting session for user {}", userId);

    chatPage->setSession(token, userId);

    contactsPresenter->setSession(token, userId);
    contactsWidget->setPresenter(contactsPresenter);
}

}  // namespace app
}  // namespace wechat
