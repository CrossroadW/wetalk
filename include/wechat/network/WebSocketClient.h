#pragma once

#include <QFuture>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <wechat/network/NetworkTypes.h>

namespace wechat {
namespace network {

/// WebSocket 客户端接口
class WebSocketClient : public QObject {
    Q_OBJECT

public:
    explicit WebSocketClient(QObject* parent = nullptr) : QObject(parent) {}
    ~WebSocketClient() override = default;

    virtual void connectToServer(const QString& url) = 0;
    virtual bool isConnected() const = 0;

    /// 请求-响应：自动分配 req_id，返回 QFuture 等待匹配响应。
    /// 超时或网络错误时 future 携带 error string。
    virtual QFuture<Result<QJsonObject>> request(QJsonObject payload) = 0;

    /// fire-and-forget：发送推送类消息（无 req_id，不等响应）
    virtual void send(QJsonObject payload) = 0;

Q_SIGNALS:
    void connected();
    void disconnected();

    /// 服务端主动推送（无 req_id 的消息）：新消息、消息变更等
    void pushReceived(const QString& type, const QJsonObject& data);

    void error(const QString& message);
};

} // namespace network
} // namespace wechat
