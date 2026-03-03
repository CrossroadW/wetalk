#pragma once

#include <wechat/core/User.h>

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

namespace wechat::network {
class WebSocketClient;
}

// Forward declaration for screen test access
class LoginScreenTest;

namespace wechat {
namespace login {

class LoginPresenter;

class LoginWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoginWidget(network::WebSocketClient* wsClient, QWidget* parent = nullptr);

    void setWebSocketClient(network::WebSocketClient* client);
    void showConnecting();  // 公共方法：显示尝试连接状态

private Q_SLOTS:
    // QR code login slots
    void onConnected();
    void onQRLoginInitResponse(const QString& sessionId, const QString& qrUrl, int expiresAt);
    void onQRScanned();
    void onQRConfirmed(int userId, const QString& username, const QString& token);
    void onTokenVerified(int userId, const QString& username);
    void onTokenInvalid();

Q_SIGNALS:
    void loggedIn(core::User user);

private:
    void setupUI();
    void setupQRLoginConnections();
    void initQRLogin();
    void showQRCode(const QString& qrUrl);
    void showLoading();
    void showScanned();
    void showEntering();
    void showDirectLogin(const QString& username);

    // Title bar widgets
    QLabel* titleLabel;
    QPushButton* settingsButton;
    QPushButton* closeButton;

    // QR code login widgets
    QLabel* qrCodeLabel;
    QLabel* qrStatusLabel;
    QPushButton* directLoginButton;
    QPushButton* cancelButton;  // 取消按钮
    QPushButton* fileTransferButton;

    network::WebSocketClient* wsClient = nullptr;
    QString currentSessionId;
    QString currentToken;
    QString currentUsername;

    friend class ::LoginScreenTest;
};

} // namespace login
} // namespace wechat
