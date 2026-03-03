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

    QApplication app(argc, argv);

    // 连接 WebSocket 后端（支持二维码登录）
    std::unique_ptr<wechat::network::NetworkClient> client;
    wechat::network::WebSocketClient* wsClient = nullptr;

    try {
        client = wechat::network::createWsClient("ws://localhost:8000/ws");
        wsClient = dynamic_cast<wechat::network::WebSocketClient*>(client.get());
        spdlog::info("Connected to WebSocket backend at ws://localhost:8000/ws");
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to backend: {}", e.what());
        spdlog::info("Please start the backend server: cd backend && uv run python main.py");
        return 1;
    }

    // ── Login (with QR code support) ──
    wechat::login::LoginPresenter loginPresenter(*client);
    wechat::login::LoginWidget loginWidget(wsClient);  // Pass WebSocket client for QR code
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
