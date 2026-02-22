#include <QApplication>
#include <QDebug>

#include <wechat/login/LoginPresenter.h>
#include <wechat/network/NetworkClient.h>

#include "../LoginWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 创建 Mock 网络客户端
    auto client = wechat::network::createMockClient();

    // 预注册一个测试用户
    client->auth().registerUser("test", "123");

    // 创建 Presenter 和 Widget
    wechat::login::LoginPresenter presenter(*client);
    wechat::login::LoginWidget widget;
    widget.setPresenter(&presenter);

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
