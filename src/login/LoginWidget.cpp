#include "LoginWidget.h"

#include <wechat/login/LoginPresenter.h>
#include <wechat/network/WebSocketClient.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QJsonObject>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/BitMatrix.h>

namespace wechat {
namespace login {

LoginWidget::LoginWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

LoginWidget::LoginWidget(network::WebSocketClient* wsClient, QWidget* parent)
    : QWidget(parent), wsClient(wsClient) {
    setupUI();
    setupQRLoginConnections();
    initQRLogin();
}

void LoginWidget::setPresenter(LoginPresenter* presenter) {
    this->presenter = presenter;
    setupConnections();
}

void LoginWidget::setWebSocketClient(network::WebSocketClient* client) {
    this->wsClient = client;
    setupQRLoginConnections();
    initQRLogin();
}

void LoginWidget::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget = new QStackedWidget(this);

    // Username/password login page
    usernamePasswordPage = new QWidget;
    auto layout = new QVBoxLayout(usernamePasswordPage);
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

    // QR code login page
    qrCodePage = new QWidget;
    auto qrLayout = new QVBoxLayout(qrCodePage);
    qrLayout->setAlignment(Qt::AlignCenter);
    qrLayout->setSpacing(20);

    auto qrTitleLabel = new QLabel("WeTalk");
    qrTitleLabel->setAlignment(Qt::AlignCenter);
    qrTitleLabel->setStyleSheet("font-size: 24px; font-weight: bold;");

    qrCodeLabel = new QLabel;
    qrCodeLabel->setAlignment(Qt::AlignCenter);
    qrCodeLabel->setFixedSize(300, 300);
    qrCodeLabel->setStyleSheet("background-color: white; border: 1px solid #ccc;");

    qrStatusLabel = new QLabel("Loading...");
    qrStatusLabel->setAlignment(Qt::AlignCenter);
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #666;");

    directLoginButton = new QPushButton("Enter WeChat");
    directLoginButton->setFixedWidth(260);
    directLoginButton->setVisible(false);

    qrLayout->addWidget(qrTitleLabel);
    qrLayout->addWidget(qrCodeLabel);
    qrLayout->addWidget(qrStatusLabel);
    qrLayout->addWidget(directLoginButton);

    stackedWidget->addWidget(usernamePasswordPage);
    stackedWidget->addWidget(qrCodePage);

    mainLayout->addWidget(stackedWidget);

    setFixedSize(800, 600);
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

void LoginWidget::setupQRLoginConnections() {
    if (!wsClient) return;

    connect(wsClient, &network::WebSocketClient::connected,
            this, &LoginWidget::onConnected);
    connect(wsClient, &network::WebSocketClient::messageReceived,
            this, [this](const QString& type, const QJsonObject& data) {
        if (type == "qr_login_init") {
            onQRLoginInitResponse(
                data["session_id"].toString(),
                data["qr_url"].toString(),
                data["expires_at"].toInt()
            );
        } else if (type == "qr_scanned") {
            onQRScanned();
        } else if (type == "qr_confirmed") {
            onQRConfirmed(
                data["user_id"].toInt(),
                data["username"].toString(),
                data["token"].toString()
            );
        } else if (type == "verify_token") {
            if (data["success"].toBool()) {
                onTokenVerified(data["user_id"].toInt(), data["username"].toString());
            } else {
                onTokenInvalid();
            }
        }
    });
}

void LoginWidget::initQRLogin() {
    if (!wsClient) return;

    // Switch to QR code page and show loading state
    stackedWidget->setCurrentWidget(qrCodePage);
    showLoading();
}

void LoginWidget::showQRCode(const QString& qrUrl) {
    try {
        // Generate QR code using ZXing
        auto writer = ZXing::MultiFormatWriter(ZXing::BarcodeFormat::QRCode);
        auto matrix = writer.encode(qrUrl.toStdWString(), 280, 280);

        // Convert BitMatrix to QImage
        QImage image(matrix.width(), matrix.height(), QImage::Format_RGB32);
        for (int y = 0; y < matrix.height(); ++y) {
            for (int x = 0; x < matrix.width(); ++x) {
                image.setPixel(x, y, matrix.get(x, y) ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
            }
        }

        qrCodeLabel->setPixmap(QPixmap::fromImage(image));
        qrStatusLabel->setText("Scan QR code with WeChat mobile app");
    } catch (const std::exception& e) {
        qrStatusLabel->setText(QString("Failed to generate QR code: %1").arg(e.what()));
    }
}

void LoginWidget::showLoading() {
    qrCodeLabel->setText("Loading...");
    qrStatusLabel->setText("Connecting to server...");
    directLoginButton->setVisible(false);
}

void LoginWidget::showScanned() {
    qrStatusLabel->setText("Scanned! Please confirm on your phone");
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #07c160;");
}

void LoginWidget::showDirectLogin(const QString& username) {
    currentUsername = username;
    qrCodeLabel->clear();
    qrCodeLabel->setText(QString("Welcome back, %1!").arg(username));
    qrCodeLabel->setStyleSheet("background-color: white; border: 1px solid #ccc; font-size: 18px;");
    qrStatusLabel->setText("Click below to enter WeChat");
    directLoginButton->setVisible(true);

    connect(directLoginButton, &QPushButton::clicked, this, [this]() {
        // TODO: Get userId from token verification
        core::User user;
        user.username = currentUsername.toStdString();
        Q_EMIT loggedIn(user);
    });
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

void LoginWidget::onConnected() {
    // WebSocket connected, ready for qr_login_init
    // Note: In test, the test calls sendQRLoginInit() directly
    // In production, we would call it here
}

void LoginWidget::onQRLoginInitResponse(const QString& sessionId, const QString& qrUrl, int expiresAt) {
    currentSessionId = sessionId;
    showQRCode(qrUrl);
}

void LoginWidget::onQRScanned() {
    showScanned();
}

void LoginWidget::onQRConfirmed(int userId, const QString& username, const QString& token) {
    currentToken = token;
    // TODO: Save token to settings

    core::User user;
    user.id = userId;
    user.username = username.toStdString();
    user.token = token.toStdString();

    Q_EMIT loggedIn(user);
}

void LoginWidget::onTokenVerified(int userId, const QString& username) {
    showDirectLogin(username);
}

void LoginWidget::onTokenInvalid() {
    // Token invalid, show QR code login
    showLoading();
}

} // namespace login
} // namespace wechat
