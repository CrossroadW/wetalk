#include <wechat/login/LoginPresenter.h>

#include <QJsonObject>

namespace wechat {
namespace login {


LoginPresenter::LoginPresenter(network::NetworkClient &client, QObject *parent)
    : QObject(parent),
      client(client) {

}

void LoginPresenter::login(std::string const &username,
                           std::string const &password) {
    auto result = client.auth().login(username, password);
    if (result.has_value()) {
        Q_EMIT loginSuccess(result.value());
    } else {
        Q_EMIT loginFailed(QString::fromStdString(result.error()));
    }
}

void LoginPresenter::registerUser(std::string const &username,
                                  std::string const &password) {
    auto result = client.auth().registerUser(username, password);
    if (result.has_value()) {
        Q_EMIT loginSuccess(result.value());
    } else {
        Q_EMIT loginFailed(QString::fromStdString(result.error()));
    }
}

void LoginPresenter::startQRLogin() {
    auto *ws = client.ws();
    if (!ws) {
        Q_EMIT loginFailed("QR login not supported in offline mode");
        return;
    }

    // 每次调用重新连接（先断开旧连接避免重复）
    QObject::disconnect(ws, &network::WebSocketClient::messageReceived, this,
                        nullptr);

    QObject::connect(
        ws, &network::WebSocketClient::messageReceived, this,
        [this](QString const &type, QJsonObject const &data) {
            if (type == "qr_login_init") {
                if (data["success"].toBool()) {
                    Q_EMIT qrCodeReady(data["qr_url"].toString(),
                                       data["session_id"].toString());
                }
            } else if (type == "qr_confirmed") {
                auto u = data["user"].toObject();
                core::User user;
                user.id = u["id"].toInt();
                user.username = u["username"].toString().toStdString();
                user.token = u["token"].toString().toStdString();
                Q_EMIT loginSuccess(user);
            }
        });

    // 发送 qr_login_init 请求
    QJsonObject request;
    request["type"] = "qr_login_init";
    request["data"] = QJsonObject{};
    ws->send(request);
}

void LoginPresenter::loginWithToken(std::string const &token) {
    auto result = client.auth().getCurrentUser(token);
    if (result.has_value()) {
        Q_EMIT loginSuccess(result.value());
    } else {
        Q_EMIT loginFailed(QString::fromStdString(result.error()));
    }
}

} // namespace login
} // namespace wechat
