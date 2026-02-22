#pragma once

#include <wechat/core/User.h>
#include <wechat/network/NetworkClient.h>

#include <QObject>
#include <QString>

#include <string>

namespace wechat {
namespace login {

class LoginPresenter : public QObject {
    Q_OBJECT

public:
    explicit LoginPresenter(network::NetworkClient& client,
                            QObject* parent = nullptr);

    void login(const std::string& username, const std::string& password);
    void registerUser(const std::string& username, const std::string& password);

Q_SIGNALS:
    void loginSuccess(core::User user);
    void loginFailed(QString errorMessage);

private:
    network::NetworkClient& client;
};

} // namespace login
} // namespace wechat
