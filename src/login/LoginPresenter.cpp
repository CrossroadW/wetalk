#include <wechat/login/LoginPresenter.h>

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

} // namespace login
} // namespace wechat
