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

    // 将顶层字段（除 type 外）全部合并进 data，方便上层统一处理
    QJsonObject data = obj[QLatin1String("data")].toObject();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() != QLatin1String("type") && it.key() != QLatin1String("data")) {
            data.insert(it.key(), it.value());
        }
    }

    Q_EMIT messageReceived(type, data);
}

void WsClient::onError(QAbstractSocket::SocketError /*error*/) {
    Q_EMIT this->error(socket_.errorString());
}

} // namespace wechat::network
