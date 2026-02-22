#include <QApplication>
#include <QDebug>

#include <wechat/contacts/ContactsPresenter.h>
#include <wechat/network/NetworkClient.h>

#include "../ContactsWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 创建 Mock 网络客户端
    auto client = wechat::network::createMockClient();

    // 预注册几个用户
    auto regA = client->auth().registerUser("me", "pass");
    client->auth().registerUser("alice", "pass");
    client->auth().registerUser("bob", "pass");
    client->auth().registerUser("bobby", "pass");
    client->auth().registerUser("carol", "pass");

    auto myToken = regA->token;
    auto myId = regA->id;

    // 预添加一个好友
    auto regAlice = client->auth().login("alice", "pass");
    client->contacts().addFriend(myToken, regAlice->id);

    // 创建 Presenter 和 Widget
    wechat::contacts::ContactsPresenter presenter(*client);
    presenter.setSession(myToken, myId);

    wechat::contacts::ContactsWidget widget;
    widget.setWindowTitle("Contacts Sandbox");
    widget.resize(400, 600);

    // 双击好友时打印
    QObject::connect(&widget, &wechat::contacts::ContactsWidget::friendSelected,
                     [](wechat::core::User user) {
                         qDebug() << "Selected friend:"
                                  << QString::fromStdString(user.username)
                                  << "id:" << user.id;
                     });

    widget.setPresenter(&presenter);
    widget.show();

    return app.exec();
}
