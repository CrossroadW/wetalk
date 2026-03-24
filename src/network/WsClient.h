#pragma once

#include <wechat/network/WebSocketClient.h>

#include <QFutureWatcher>
#include <QPromise>
#include <QWebSocket>

#include <atomic>
#include <map>
#include <memory>

namespace wechat {
namespace network {

/// 真实 WebSocket 客户端实现
///
/// request() 给每个请求分配唯一 req_id，内部维护 pending map，
/// 收到带 req_id 的响应时 resolve 对应 QPromise。
/// 不带 req_id 的消息作为服务端推送，通过 pushReceived 信号广播。
class WsClient : public WebSocketClient {
    Q_OBJECT

public:
    explicit WsClient(QObject* parent = nullptr);

    void connectToServer(const QString& url) override;
    bool isConnected() const override;

    QFuture<Result<QJsonObject>> request(QJsonObject payload) override;
    void send(QJsonObject payload) override;

private Q_SLOTS:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket socket_;
    std::atomic<int64_t> nextReqId_{0};

    // req_id → pending promise（shared_ptr 保证跨线程安全传递）
    std::map<QString, std::shared_ptr<QPromise<Result<QJsonObject>>>> pending_;
};

} // namespace network
} // namespace wechat
