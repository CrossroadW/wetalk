#include "MainWindow.h"

#include <wechat/core/AppPaths.h>
#include <wechat/log/Log.h>
#include <wechat/login/LoginPresenter.h>
#include <wechat/network/NetworkClient.h>

#include "../login/LoginWidget.h"

#include <QApplication>

#include <spdlog/spdlog.h>

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

    // ── Main Window ──
    auto* mainWindow = new wechat::app::MainWindow(*client);

    // 登录成功 → 切换到主窗口
    QObject::connect(&loginWidget, &wechat::login::LoginWidget::loggedIn,
                     [mainWindow](wechat::core::User user) {
                         spdlog::info("Logged in as {} (id={})",
                                      user.username, user.id);
                         mainWindow->setSession(user.token, user.id);
                         mainWindow->show();
                     });

    // 登录窗口关闭时也关闭主窗口
    QObject::connect(&loginWidget, &QWidget::destroyed, mainWindow,
                     &QWidget::close);

    loginWidget.show();

    spdlog::info("WeTalk started");
    return app.exec();
}
