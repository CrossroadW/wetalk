#include "LoginWidget.h"

#include <wechat/login/LoginPresenter.h>
#include <wechat/network/WebSocketClient.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QJsonObject>
#include <QIcon>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/BitMatrix.h>

namespace wechat {
namespace login {

LoginWidget::LoginWidget(network::WebSocketClient* wsClient, QWidget* parent)
    : QWidget(parent), wsClient(wsClient) {
    setupUI();
    setupQRLoginConnections();
    initQRLogin();
}

void LoginWidget::setWebSocketClient(network::WebSocketClient* client) {
    this->wsClient = client;
    setupQRLoginConnections();
    initQRLogin();
}

void LoginWidget::setupUI() {
    // 设置窗口背景色 - ChatFlow 浅色主题
    setStyleSheet("QWidget { background-color: #F5F5F5; }");

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 顶部标题栏 ==========
    auto titleBar = new QWidget;
    titleBar->setFixedHeight(50);
    titleBar->setStyleSheet("background-color: #FFFFFF; border-bottom: 1px solid #E0E0E0;");

    auto titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 20, 0);

    // 应用名称
    titleLabel = new QLabel("畅聊");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333333;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    // 设置按钮
    settingsButton = new QPushButton;
    settingsButton->setIcon(QIcon(":/icons/settings.svg"));
    settingsButton->setIconSize(QSize(20, 20));
    settingsButton->setFixedSize(36, 36);
    settingsButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: none; border-radius: 18px; }"
        "QPushButton:hover { background-color: #F0F0F0; }"
        "QPushButton:pressed { background-color: #E0E0E0; }"
    );
    settingsButton->setCursor(Qt::PointingHandCursor);

    // 关闭按钮
    closeButton = new QPushButton;
    closeButton->setIcon(QIcon(":/icons/close.svg"));
    closeButton->setIconSize(QSize(20, 20));
    closeButton->setFixedSize(36, 36);
    closeButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: none; border-radius: 18px; }"
        "QPushButton:hover { background-color: #FFEBEE; }"
        "QPushButton:pressed { background-color: #FFCDD2; }"
    );
    closeButton->setCursor(Qt::PointingHandCursor);
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    titleLayout->addWidget(settingsButton);
    titleLayout->addWidget(closeButton);

    mainLayout->addWidget(titleBar);

    // ========== 中间内容区域 ==========
    auto contentWidget = new QWidget;
    auto contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setAlignment(Qt::AlignCenter);
    contentLayout->setSpacing(20);
    contentLayout->setContentsMargins(40, 40, 40, 40);

    // 二维码区域
    qrCodeLabel = new QLabel;
    qrCodeLabel->setAlignment(Qt::AlignCenter);
    qrCodeLabel->setFixedSize(300, 300);
    qrCodeLabel->setStyleSheet(
        "background-color: #FFFFFF; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 8px;"
    );

    // 状态文字
    qrStatusLabel = new QLabel("加载中...");
    qrStatusLabel->setAlignment(Qt::AlignCenter);
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #666666;");

    // 直接登录按钮
    directLoginButton = new QPushButton("进入畅聊");
    directLoginButton->setFixedWidth(260);
    directLoginButton->setFixedHeight(40);
    directLoginButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #5B9FED; "
        "  color: #FFFFFF; "
        "  border: none; "
        "  border-radius: 20px; "
        "  font-size: 14px; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #6AAFFD; }"
        "QPushButton:pressed { background-color: #4B8FDD; }"
    );
    directLoginButton->setCursor(Qt::PointingHandCursor);
    directLoginButton->setVisible(false);

    contentLayout->addWidget(qrCodeLabel);
    contentLayout->addWidget(qrStatusLabel);
    contentLayout->addWidget(directLoginButton);

    // 取消按钮（用于正在进入状态）
    cancelButton = new QPushButton("取消");
    cancelButton->setStyleSheet(
        "QPushButton { "
        "  background-color: transparent; "
        "  color: #999999; "
        "  border: none; "
        "  font-size: 14px; "
        "}"
        "QPushButton:hover { color: #666666; }"
    );
    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setVisible(false);
    contentLayout->addWidget(cancelButton);

    mainLayout->addWidget(contentWidget, 1);

    // ========== 底部文件传输按钮 ==========
    auto bottomWidget = new QWidget;
    bottomWidget->setFixedHeight(60);
    auto bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 20);
    bottomLayout->setAlignment(Qt::AlignCenter);

    fileTransferButton = new QPushButton("仅传输文件");
    fileTransferButton->setStyleSheet(
        "QPushButton { "
        "  background-color: transparent; "
        "  color: #999999; "
        "  border: none; "
        "  font-size: 13px; "
        "}"
        "QPushButton:hover { color: #5B9FED; }"
    );
    fileTransferButton->setCursor(Qt::PointingHandCursor);

    bottomLayout->addWidget(fileTransferButton);
    mainLayout->addWidget(bottomWidget);

    setFixedSize(800, 600);
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

    // Show loading state
    showLoading();

    // If already connected, send qr_login_init immediately
    if (wsClient->isConnected()) {
        QJsonObject req;
        req["type"] = "qr_login_init";
        req["data"] = QJsonObject{};
        wsClient->send(req);
    }
    // Otherwise, onConnected() will send it when connection is established
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
        qrStatusLabel->setText("请使用手机扫描二维码登录");
        qrStatusLabel->setStyleSheet("font-size: 14px; color: #5B9FED;");  // ChatFlow 蓝色
    } catch (const std::exception& e) {
        qrStatusLabel->setText(QString("Failed to generate QR code: %1").arg(e.what()));
        qrStatusLabel->setStyleSheet("font-size: 14px; color: #E53935;");  // 错误红色
    }
}

void LoginWidget::showLoading() {
    qrCodeLabel->setText("加载中...");
    qrCodeLabel->setStyleSheet(
        "background-color: #FFFFFF; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 8px; "
        "color: #999999; "
        "font-size: 16px;"
    );
    qrStatusLabel->setText("连接服务器中...");
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #666666;");
    directLoginButton->setVisible(false);
    cancelButton->setVisible(false);
}

void LoginWidget::showScanned() {
    qrStatusLabel->setText("已扫描！请在手机上确认");
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #5B9FED;");  // ChatFlow 蓝色
    cancelButton->setVisible(false);
}

void LoginWidget::showEntering() {
    qrCodeLabel->setText("加载中...");
    qrCodeLabel->setStyleSheet(
        "background-color: #FFFFFF; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 8px; "
        "color: #999999; "
        "font-size: 16px;"
    );
    qrStatusLabel->setText("正在进入");
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #07C160;");  // 微信绿色
    directLoginButton->setVisible(false);
    cancelButton->setVisible(true);
}

void LoginWidget::showDirectLogin(const QString& username) {
    currentUsername = username;
    qrCodeLabel->clear();
    qrCodeLabel->setText(QString("欢迎回来，%1！").arg(username));
    qrCodeLabel->setStyleSheet(
        "background-color: #FFFFFF; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 8px; "
        "font-size: 18px; "
        "color: #333333;"
    );
    qrStatusLabel->setText("点击下方按钮进入");
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #666666;");
    directLoginButton->setVisible(true);
    cancelButton->setVisible(false);

    connect(directLoginButton, &QPushButton::clicked, this, [this]() {
        // TODO: Get userId from token verification
        core::User user;
        user.username = currentUsername.toStdString();
        Q_EMIT loggedIn(user);
    });
}

void LoginWidget::showConnecting() {
    qrCodeLabel->setText("加载中...");
    qrCodeLabel->setStyleSheet(
        "background-color: #FFFFFF; "
        "border: 1px solid #E0E0E0; "
        "border-radius: 8px; "
        "color: #999999; "
        "font-size: 16px;"
    );
    qrStatusLabel->setText("连接服务器中...");
    qrStatusLabel->setStyleSheet("font-size: 14px; color: #666666;");
    directLoginButton->setVisible(false);
    cancelButton->setVisible(false);
}

void LoginWidget::onConnected() {
    // WebSocket connected, send qr_login_init to get QR code
    if (!wsClient) return;

    QJsonObject req;
    req["type"] = "qr_login_init";
    req["data"] = QJsonObject{};
    wsClient->send(req);
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

    // 显示正在进入状态
    showEntering();

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
