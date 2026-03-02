#include "WsClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace wechat::network {

WsClient::WsClient(QObject* parent) : WebSocketClient(parent) {
    QObject::connect(&socket_, &QWebSocket::connected,
                     this, &WsClient::onConnected);
    QObject::connect(&socket_, &QWebSocket::disconnected,
                     this, &WsClient::onDisconnected);
    QObject::connect(&socket_, &QWebSocket::textMessageReceived,
                     this, &WsClient::onTextMessageReceived);
    QObject::connect(&socket_, &QWebSocket::errorOccurred,
                     this, &WsClient::onError);
}

void WsClient::connectToServer(const QString& url) {
    socket_.open(QUrl(url));
}

void WsClient::send(const QJsonObject& message) {
    QJsonDocument doc(message);
    socket_.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

bool WsClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

void WsClient::onConnected() {
    Q_EMIT connected();
}

void WsClient::onDisconnected() {
    Q_EMIT disconnected();
}

void WsClient::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj[QLatin1String("type")].toString();
    QJsonObject data = obj[QLatin1String("data")].toObject();
    // 后端响应格式: { type, success, data }
    // 将 success 合并进 data，方便上层统一处理
    if (obj.contains(QLatin1String("success"))) {
        data.insert(QLatin1String("success"), obj[QLatin1String("success")]);
    }
    Q_EMIT messageReceived(type, data);
}

void WsClient::onError(QAbstractSocket::SocketError /*error*/) {
    Q_EMIT this->error(socket_.errorString());
}

} // namespace wechat::network
