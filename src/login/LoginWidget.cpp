#include "LoginWidget.h"

#include <wechat/login/LoginPresenter.h>

#include <QVBoxLayout>

namespace wechat {
namespace login {

LoginWidget::LoginWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void LoginWidget::setPresenter(LoginPresenter* presenter) {
    this->presenter = presenter;
    setupConnections();
}

void LoginWidget::setupUI() {
    auto layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    auto titleLabel = new QLabel("WeTalk");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin-bottom: 20px;");

    usernameInput = new QLineEdit;
    usernameInput->setPlaceholderText("Username");
    usernameInput->setFixedWidth(260);

    passwordInput = new QLineEdit;
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setFixedWidth(260);

    submitButton = new QPushButton("Login");
    submitButton->setFixedWidth(260);

    toggleButton = new QPushButton("Switch to Register");
    toggleButton->setFlat(true);
    toggleButton->setFixedWidth(260);

    statusLabel = new QLabel;
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: red;");

    layout->addWidget(titleLabel);
    layout->addWidget(usernameInput);
    layout->addWidget(passwordInput);
    layout->addWidget(submitButton);
    layout->addWidget(toggleButton);
    layout->addWidget(statusLabel);

    setFixedSize(360, 400);
}

void LoginWidget::setupConnections() {
    connect(submitButton, &QPushButton::clicked, this, &LoginWidget::onSubmit);
    connect(toggleButton, &QPushButton::clicked, this, &LoginWidget::onToggleMode);
    connect(passwordInput, &QLineEdit::returnPressed, this, &LoginWidget::onSubmit);

    if (presenter) {
        connect(presenter, &LoginPresenter::loginSuccess,
                this, &LoginWidget::onLoginSuccess);
        connect(presenter, &LoginPresenter::loginFailed,
                this, &LoginWidget::onLoginFailed);
    }
}

void LoginWidget::onSubmit() {
    if (!presenter) return;

    auto username = usernameInput->text().toStdString();
    auto password = passwordInput->text().toStdString();
    statusLabel->clear();

    if (registerMode) {
        presenter->registerUser(username, password);
    } else {
        presenter->login(username, password);
    }
}

void LoginWidget::onToggleMode() {
    registerMode = !registerMode;
    submitButton->setText(registerMode ? "Register" : "Login");
    toggleButton->setText(registerMode ? "Switch to Login" : "Switch to Register");
    statusLabel->clear();
}

void LoginWidget::onLoginSuccess(core::User user) {
    Q_EMIT loggedIn(user);
}

void LoginWidget::onLoginFailed(QString message) {
    statusLabel->setText(message);
}

} // namespace login
} // namespace wechat
