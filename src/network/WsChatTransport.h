#pragma once

#include <wechat/network/ChatTransport.h>
#include <wechat/network/WebSocketClient.h>

namespace wechat {
namespace network {

/// 基于 WebSocket 的 ChatTransport 实现
///
/// fetch* 方法用 WsClient.request() → QFuture，完全非阻塞。
/// send/revoke/edit 用 WsClient.send()，fire-and-forget。
/// 后端推送经 pushReceived 信号解析后 emit messagePushed / messageChangePushed。
class WsChatTransport : public ChatTransport {
    Q_OBJECT

public:
    explicit WsChatTransport(WebSocketClient& ws, QObject* parent = nullptr);

    QFuture<Result<SyncResponse>> fetchLatest(
        const std::string& token, int64_t chatId, int limit) override;

    QFuture<Result<SyncResponse>> fetchBefore(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int limit) override;

    QFuture<Result<SyncResponse>> fetchAfter(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int limit) override;

    QFuture<Result<SyncResponse>> fetchAround(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int radius) override;

    QFuture<Result<SyncResponse>> fetchUpdated(
        const std::string& token, int64_t chatId,
        int64_t maxUpdatedAt, int limit) override;

    void sendMessage(const std::string& token, int64_t chatId,
                     int64_t replyTo,
                     const core::MessageContent& content) override;

    void revokeMessage(const std::string& token, int64_t msgId) override;

    void editMessage(const std::string& token, int64_t msgId,
                     const core::MessageContent& content) override;

private Q_SLOTS:
    void onPushReceived(const QString& type, const QJsonObject& data);

private:
    WebSocketClient& ws_;

    /// 将 ws.request() 返回的 QFuture<Result<QJsonObject>> 转换为
    /// QFuture<Result<SyncResponse>>
    static QFuture<Result<SyncResponse>> toSyncResponse(
        QFuture<Result<QJsonObject>> future);

    static core::Message parseMessage(const QJsonObject& obj);
};

} // namespace network
} // namespace wechat
