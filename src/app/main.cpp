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

    // 创建 LoginWidget（先显示，连接失败时显示重试状态）
    wechat::login::LoginWidget* loginWidget = nullptr;

    // 重试连接的定时器
    auto* retryTimer = new QTimer(&app);
    retryTimer->setInterval(1000);  // 1秒重试一次

    // 连接函数
    auto tryConnect = [&]() -> bool {
        try {
            client = wechat::network::createWsClient("ws://127.0.0.1:8000/ws");
            wsClient = dynamic_cast<wechat::network::WebSocketClient*>(client.get());
            spdlog::info("Connected to WebSocket backend at ws://127.0.0.1:8000/ws");
            return true;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to connect to backend: {}", e.what());
            return false;
        }
    };

    // 首次尝试连接
    bool connected = tryConnect();

    if (!connected) {
        spdlog::info("Backend not available, will retry every 1 second...");
        spdlog::info("Please start the backend server: cd backend && uv run python main.py");

        // 创建 LoginWidget 并显示连接失败状态
        loginWidget = new wechat::login::LoginWidget(nullptr);
        loginWidget->setWindowTitle("WeTalk - Login");
        loginWidget->showConnectionFailed();  // 显示连接失败状态
        loginWidget->show();

        // 启动重试定时器
        QObject::connect(retryTimer, &QTimer::timeout, [&]() {
            if (tryConnect()) {
                retryTimer->stop();
                spdlog::info("Connection established, initializing login widget...");

                // 连接成功，更新 LoginWidget
                loginWidget->setWebSocketClient(wsClient);

                // 创建主窗口
                auto* mainWindow = new wechat::app::MainWindow(*client);

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
            }
        });
        retryTimer->start();

    } else {
        // 连接成功，正常启动
        loginWidget = new wechat::login::LoginWidget(wsClient);
        loginWidget->setWindowTitle("WeTalk - Login");

        // ── Main Window ──
        auto* mainWindow = new wechat::app::MainWindow(*client);

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

        loginWidget->show();
    }

    spdlog::info("WeTalk started");
    return app.exec();
}
