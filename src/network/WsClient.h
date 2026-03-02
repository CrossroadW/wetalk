#pragma once

#include <wechat/network/WebSocketClient.h>

#include <QWebSocket>

namespace wechat::network {

/// 真实 WebSocket 客户端实现
class WsClient : public WebSocketClient {
    Q_OBJECT

public:
    explicit WsClient(QObject* parent = nullptr);

    void connectToServer(const QString& url) override;
    void send(const QJsonObject& message) override;
    bool isConnected() const override;

private Q_SLOTS:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket socket_;
};

} // namespace wechat::network
