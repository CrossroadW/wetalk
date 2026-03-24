#include "WsClient.h"

#include <QJsonDocument>

namespace wechat {
namespace network {

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

bool WsClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

QFuture<Result<QJsonObject>> WsClient::request(QJsonObject payload) {
    auto promise = std::make_shared<QPromise<Result<QJsonObject>>>();
    promise->start();

    QString reqId = QString::number(nextReqId_.fetch_add(1));
    payload[QLatin1String("req_id")] = reqId;
    pending_[reqId] = promise;

    QJsonDocument doc(payload);
    socket_.sendTextMessage(doc.toJson(QJsonDocument::Compact));

    return promise->future();
}

void WsClient::send(QJsonObject payload) {
    QJsonDocument doc(payload);
    socket_.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void WsClient::onConnected() {
    Q_EMIT connected();
}

void WsClient::onDisconnected() {
    // 连接断开时，所有 pending 请求以错误结束
    for (auto& [id, promise] : pending_) {
        promise->addResult(fail("disconnected"));
        promise->finish();
    }
    pending_.clear();
    Q_EMIT disconnected();
}

void WsClient::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();

    if (obj.contains(QLatin1String("req_id"))) {
        // 请求-响应：找到对应的 promise 并 resolve
        QString reqId = obj[QLatin1String("req_id")].toString();
        auto it = pending_.find(reqId);
        if (it != pending_.end()) {
            auto promise = std::move(it->second);
            pending_.erase(it);

            if (obj.contains(QLatin1String("error"))) {
                promise->addResult(fail(obj[QLatin1String("error")]
                                           .toString().toStdString()));
            } else {
                promise->addResult(obj);
            }
            promise->finish();
        }
    } else {
        // 服务端主动推送：合并顶层字段后广播
        QString type = obj[QLatin1String("type")].toString();
        QJsonObject data = obj[QLatin1String("data")].toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it.key() != QLatin1String("type") &&
                it.key() != QLatin1String("data")) {
                data.insert(it.key(), it.value());
            }
        }
        Q_EMIT pushReceived(type, data);
    }
}

void WsClient::onError(QAbstractSocket::SocketError /*error*/) {
    Q_EMIT this->error(socket_.errorString());
}

} // namespace network
} // namespace wechat
