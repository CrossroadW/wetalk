#include "MainWindow.h"

#include <wechat/core/AppPaths.h>
#include <wechat/log/Log.h>
#include <wechat/network/NetworkClient.h>

#include "../login/LoginWidget.h"

#include <QApplication>
#include <QTimer>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    wechat::log::init();

    QApplication app(argc, argv);

    // 连接 WebSocket 后端（二维码登录）
    std::unique_ptr<wechat::network::NetworkClient> client;
    wechat::network::WebSocketClient* wsClient = nullptr;

    // 创建 LoginWidget 并立即显示（无论是否连接成功）
    auto* loginWidget = new wechat::login::LoginWidget(nullptr);
    loginWidget->setWindowTitle("畅聊 - 登录");
    loginWidget->showConnecting();  // 显示尝试连接状态
    loginWidget->show();

    // 主窗口（延迟创建，连接成功后创建）
    wechat::app::MainWindow* mainWindow = nullptr;

    // 重试连接的定时器
    auto* retryTimer = new QTimer(&app);
    retryTimer->setInterval(1000);  // 1秒重试一次

    // 连接函数
    auto tryConnect = [&]() -> bool {
        try {
            client = wechat::network::createWsClient("ws://127.0.0.1:8000/ws");
            wsClient = client->ws();  // 使用 ws() 方法获取 WebSocket 客户端
            spdlog::info("Connected to WebSocket backend at ws://127.0.0.1:8000/ws");
            return true;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to connect to backend: {}", e.what());
            return false;
        }
    };

    // 连接成功后的处理
    auto onConnected = [&]() {
        retryTimer->stop();
        spdlog::info("Connection established, initializing login widget...");

        // 连接成功，更新 LoginWidget
        loginWidget->setWebSocketClient(wsClient);

        // 创建主窗口
        mainWindow = new wechat::app::MainWindow(*client);

        // 登录成功 → 切换到主窗口
        QObject::connect(loginWidget, &wechat::login::LoginWidget::loggedIn,
                         [mainWindow](wechat::core::User user) {
                             spdlog::info("Logged in as {} (id={})",
                                          user.username, user.id);
                             mainWindow->setSession(user.token, user.id);
                             mainWindow->show();
                         });

        // 登录窗口关闭时也关闭主窗口
        QObject::connect(loginWidget, &QWidget::destroyed, mainWindow,
                         &QWidget::close);
    };

    // 首次尝试连接
    if (tryConnect()) {
        onConnected();
    } else {
        spdlog::info("Backend not available, will retry every 1 second...");
        spdlog::info("Please start the backend server: cd backend && uv run python main.py");

        // 启动重试定时器
        QObject::connect(retryTimer, &QTimer::timeout, [&]() {
            if (tryConnect()) {
                onConnected();
            }
        });
        retryTimer->start();
    }

    spdlog::info("ChatFlow started");
    return app.exec();
}
