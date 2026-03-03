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
    void showConnectionFailed();  // 公共方法：显示连接失败状态

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
    void showDirectLogin(const QString& username);

    // QR code login widgets
    QLabel* qrCodeLabel;
    QLabel* qrStatusLabel;
    QPushButton* directLoginButton;

    network::WebSocketClient* wsClient = nullptr;
    QString currentSessionId;
    QString currentToken;
    QString currentUsername;

    friend class ::LoginScreenTest;
};

} // namespace login
} // namespace wechat
