#pragma once

#include <wechat/core/User.h>

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

namespace wechat {
namespace login {

class LoginPresenter;

class LoginWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoginWidget(QWidget* parent = nullptr);

    void setPresenter(LoginPresenter* presenter);

private Q_SLOTS:
    void onSubmit();
    void onToggleMode();
    void onLoginSuccess(core::User user);
    void onLoginFailed(QString message);

Q_SIGNALS:
    void loggedIn(core::User user);

private:
    void setupUI();
    void setupConnections();

    QLineEdit* usernameInput;
    QLineEdit* passwordInput;
    QPushButton* submitButton;
    QPushButton* toggleButton;
    QLabel* statusLabel;

    LoginPresenter* presenter = nullptr;
    bool registerMode = false;
};

} // namespace login
} // namespace wechat
