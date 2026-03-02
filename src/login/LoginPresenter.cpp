#include <wechat/login/LoginPresenter.h>

#include <QDebug>

namespace wechat {
namespace login {

LoginPresenter::LoginPresenter(network::NetworkClient& client, QObject* parent)
    : QObject(parent), client(client) {}

void LoginPresenter::login(const std::string& username,
                           const std::string& password) {
    auto result = client.auth().login(username, password);
    if (result.has_value()) {
        Q_EMIT loginSuccess(result.value());
    } else {
        Q_EMIT loginFailed(QString::fromStdString(result.error()));
    }
}

void LoginPresenter::registerUser(const std::string& username,
                                  const std::string& password) {
    auto result = client.auth().registerUser(username, password);
    if (result.has_value()) {
        Q_EMIT loginSuccess(result.value());
    } else {
        Q_EMIT loginFailed(QString::fromStdString(result.error()));
    }
}

void LoginPresenter::startQRLogin() {
    // TODO: 实现二维码登录
    // 1. 通过 WebSocket 发送 qr_login_init 请求
    // 2. 接收 session_id 和 qr_url
    // 3. Q_EMIT qrCodeReady(qrUrl, sessionId)
    // 4. 监听 qr_confirmed 推送
    // 5. Q_EMIT loginSuccess(user)
    qDebug() << "LoginPresenter::startQRLogin() - TODO: implement";
}

void LoginPresenter::loginWithToken(const std::string& token) {
    // TODO: 实现 token 登录
    // 调用 AuthService::getCurrentUser(token)
    auto result = client.auth().getCurrentUser(token);
    if (result.has_value()) {
        Q_EMIT loginSuccess(result.value());
    } else {
        Q_EMIT loginFailed(QString::fromStdString(result.error()));
    }
}

} // namespace login
} // namespace wechat
