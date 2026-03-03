#include <QApplication>
#include <QDebug>

#include "../../network/WsClient.h"
#include "../LoginWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 创建 WebSocket 客户端并连接到后端
    auto* wsClient = new wechat::network::WsClient(&app);
    wsClient->connectToServer("ws://127.0.0.1:8000/ws");

    // 创建 LoginWidget（自动显示二维码登录界面）
    wechat::login::LoginWidget widget(wsClient);

    // 登录成功后打印用户信息并退出
    QObject::connect(&widget, &wechat::login::LoginWidget::loggedIn,
                     [](wechat::core::User user) {
                         qDebug() << "Login success!"
                                  << "id:" << user.id
                                  << "username:" << QString::fromStdString(user.username)
                                  << "token:" << QString::fromStdString(user.token);
                         QApplication::quit();
                     });

    widget.show();
    return app.exec();
}
