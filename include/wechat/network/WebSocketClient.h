#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>

namespace wechat::network {

/// WebSocket 客户端接口
/// 用于与后端 WebSocket 服务器通信
class WebSocketClient : public QObject {
    Q_OBJECT

public:
    explicit WebSocketClient(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~WebSocketClient() = default;

    /// 连接到 WebSocket 服务器
    virtual void connectToServer(const QString& url) = 0;

    /// 发送 JSON 消息
    virtual void send(const QJsonObject& message) = 0;

    /// 检查是否已连接
    virtual bool isConnected() const = 0;

    /// 发起二维码登录请求
    virtual void sendQrLoginInit() = 0;

Q_SIGNALS:
    /// 连接成功
    void connected();

    /// 连接断开
    void disconnected();

    /// 收到消息（type 是消息类型，data 是消息数据）
    void messageReceived(const QString& type, const QJsonObject& data);

    /// 发生错误
    void error(const QString& errorMessage);

    /// 收到二维码登录初始化响应（sessionId, qrUrl）
    void qrLoginInitReceived(const QString& sessionId, const QString& qrUrl);

    /// 收到二维码登录确认（token）
    void qrLoginConfirmed(const QString& token);
};

} // namespace wechat::network
