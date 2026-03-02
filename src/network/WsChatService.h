#pragma once

#include <wechat/network/ChatService.h>
#include <wechat/network/NetworkTypes.h>
#include <wechat/network/WebSocketClient.h>

namespace wechat {
namespace network {

/// 基于 WebSocket 的聊天服务实现
class WsChatService : public ChatService {
    Q_OBJECT

public:
    explicit WsChatService(WebSocketClient& ws, QObject* parent = nullptr);

    Result<core::Message> sendMessage(
        const std::string& token,
        int64_t chatId,
        int64_t replyTo,
        const core::MessageContent& content) override;

    Result<SyncMessagesResponse> fetchAfter(
        const std::string& token,
        int64_t chatId,
        int64_t afterId,
        int limit) override;

    Result<SyncMessagesResponse> fetchBefore(
        const std::string& token,
        int64_t chatId,
        int64_t beforeId,
        int limit) override;

    Result<SyncMessagesResponse> fetchUpdated(
        const std::string& token,
        int64_t chatId,
        int64_t startId,
        int64_t endId,
        int64_t updatedAt,
        int limit) override;

    VoidResult revokeMessage(
        const std::string& token,
        int64_t messageId) override;

    VoidResult editMessage(
        const std::string& token,
        int64_t messageId,
        const core::MessageContent& newContent) override;

    VoidResult markRead(
        const std::string& token,
        int64_t chatId,
        int64_t lastMessageId) override;

private:
    WebSocketClient& ws;
};

} // namespace network
} // namespace wechat
