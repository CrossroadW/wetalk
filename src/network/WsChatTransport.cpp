#include "WsChatTransport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPromise>

#include "../cache/MessageJson.h"

namespace wechat {
namespace network {

WsChatTransport::WsChatTransport(WebSocketClient& ws, QObject* parent)
    : ChatTransport(parent), ws_(ws) {
    connect(&ws_, &WebSocketClient::pushReceived,
            this, &WsChatTransport::onPushReceived);
}

// ── 读操作 ──

QFuture<Result<SyncResponse>> WsChatTransport::fetchLatest(
    const std::string& token, int64_t chatId, int limit) {

    QJsonObject payload;
    payload["type"]    = "fetch_latest";
    payload["token"]   = QString::fromStdString(token);
    payload["chat_id"] = chatId;
    payload["limit"]   = limit;
    return toSyncResponse(ws_.request(payload));
}

QFuture<Result<SyncResponse>> WsChatTransport::fetchBefore(
    const std::string& token, int64_t chatId,
    int32_t chatSeq, int limit) {

    QJsonObject payload;
    payload["type"]     = "fetch_before";
    payload["token"]    = QString::fromStdString(token);
    payload["chat_id"]  = chatId;
    payload["chat_seq"] = chatSeq;
    payload["limit"]    = limit;
    return toSyncResponse(ws_.request(payload));
}

QFuture<Result<SyncResponse>> WsChatTransport::fetchAfter(
    const std::string& token, int64_t chatId,
    int32_t chatSeq, int limit) {

    QJsonObject payload;
    payload["type"]     = "fetch_after";
    payload["token"]    = QString::fromStdString(token);
    payload["chat_id"]  = chatId;
    payload["chat_seq"] = chatSeq;
    payload["limit"]    = limit;
    return toSyncResponse(ws_.request(payload));
}

QFuture<Result<SyncResponse>> WsChatTransport::fetchAround(
    const std::string& token, int64_t chatId,
    int32_t chatSeq, int radius) {

    QJsonObject payload;
    payload["type"]     = "fetch_around";
    payload["token"]    = QString::fromStdString(token);
    payload["chat_id"]  = chatId;
    payload["chat_seq"] = chatSeq;
    payload["radius"]   = radius;
    return toSyncResponse(ws_.request(payload));
}

QFuture<Result<SyncResponse>> WsChatTransport::fetchUpdated(
    const std::string& token, int64_t chatId,
    int64_t maxUpdatedAt, int limit) {

    QJsonObject payload;
    payload["type"]           = "fetch_updated";
    payload["token"]          = QString::fromStdString(token);
    payload["chat_id"]        = chatId;
    payload["max_updated_at"] = maxUpdatedAt;
    payload["limit"]          = limit;
    return toSyncResponse(ws_.request(payload));
}

// ── 写操作 ──

void WsChatTransport::sendMessage(
    const std::string& token, int64_t chatId,
    int64_t replyTo, const core::MessageContent& content) {

    using namespace cache::detail;
    QJsonObject payload;
    payload["type"]     = "send_message";
    payload["token"]    = QString::fromStdString(token);
    payload["chat_id"]  = chatId;
    payload["reply_to"] = replyTo;
    payload["content"]  = QString::fromStdString(serializeContent(content));
    ws_.send(payload);
}

void WsChatTransport::revokeMessage(
    const std::string& token, int64_t msgId) {

    QJsonObject payload;
    payload["type"]   = "revoke_message";
    payload["token"]  = QString::fromStdString(token);
    payload["msg_id"] = msgId;
    ws_.send(payload);
}

void WsChatTransport::editMessage(
    const std::string& token, int64_t msgId,
    const core::MessageContent& content) {

    using namespace cache::detail;
    QJsonObject payload;
    payload["type"]    = "edit_message";
    payload["token"]   = QString::fromStdString(token);
    payload["msg_id"]  = msgId;
    payload["content"] = QString::fromStdString(serializeContent(content));
    ws_.send(payload);
}

// ── 推送处理 ──

void WsChatTransport::onPushReceived(const QString& type,
                                      const QJsonObject& data) {
    if (type == "new_message") {
        auto msg = parseMessage(data["message"].toObject());
        Q_EMIT messagePushed(msg.chatId, msg);
    } else if (type == "message_edited" || type == "message_revoked") {
        auto msg = parseMessage(data["message"].toObject());
        Q_EMIT messageChangePushed(msg.chatId, msg);
    }
}

// ── 内部工具 ──

core::Message WsChatTransport::parseMessage(const QJsonObject& obj) {
    using namespace cache::detail;
    core::Message msg;
    msg.id        = obj["id"].toInteger();
    msg.chatSeq   = static_cast<int32_t>(obj["chat_seq"].toInteger());
    msg.senderId  = obj["sender_id"].toInteger();
    msg.chatId    = obj["chat_id"].toInteger();
    msg.replyTo   = obj["reply_to"].toInteger();
    msg.timestamp = obj["timestamp"].toInteger();
    msg.editedAt  = obj["edited_at"].toInteger();
    msg.revoked   = obj["revoked"].toBool();
    msg.readCount = static_cast<uint32_t>(obj["read_count"].toInt());
    msg.updatedAt = obj["updated_at"].toInteger();

    auto contentStr = obj["content"].toString();
    if (!contentStr.isEmpty())
        msg.content = deserializeContent(contentStr.toStdString());

    return msg;
}

QFuture<Result<SyncResponse>> WsChatTransport::toSyncResponse(
    QFuture<Result<QJsonObject>> future) {

    return future.then([](Result<QJsonObject> result) -> Result<SyncResponse> {
        if (!result) return std::unexpected(result.error());

        const QJsonObject& obj = result.value();
        SyncResponse resp;
        resp.hasMore = obj["has_more"].toBool();

        for (const auto& item : obj["messages"].toArray()) {
            resp.messages.push_back(parseMessage(item.toObject()));
        }
        return resp;
    });
}

} // namespace network
} // namespace wechat
