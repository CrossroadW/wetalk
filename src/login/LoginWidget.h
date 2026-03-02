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

namespace wechat {
namespace login {

class LoginPresenter;

class LoginWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoginWidget(QWidget* parent = nullptr);
    explicit LoginWidget(network::WebSocketClient* wsClient, QWidget* parent = nullptr);

    void setPresenter(LoginPresenter* presenter);
    void setWebSocketClient(network::WebSocketClient* client);

private Q_SLOTS:
    void onSubmit();
    void onToggleMode();
    void onLoginSuccess(core::User user);
    void onLoginFailed(QString message);

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
    void setupConnections();
    void setupQRLoginConnections();
    void initQRLogin();
    void showQRCode(const QString& qrUrl);
    void showLoading();
    void showScanned();
    void showDirectLogin(const QString& username);

    // Username/password login widgets
    QLineEdit* usernameInput;
    QLineEdit* passwordInput;
    QPushButton* submitButton;
    QPushButton* toggleButton;
    QLabel* statusLabel;

    // QR code login widgets
    QStackedWidget* stackedWidget;
    QWidget* usernamePasswordPage;
    QWidget* qrCodePage;
    QLabel* qrCodeLabel;
    QLabel* qrStatusLabel;
    QPushButton* directLoginButton;

    LoginPresenter* presenter = nullptr;
    network::WebSocketClient* wsClient = nullptr;
    bool registerMode = false;
    QString currentSessionId;
    QString currentToken;
    QString currentUsername;
};

} // namespace login
} // namespace wechat
