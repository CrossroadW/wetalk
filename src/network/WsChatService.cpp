#include "WsChatService.h"

namespace wechat {
namespace network {

WsChatService::WsChatService(WebSocketClient& ws, QObject* parent)
    : ChatService(parent), ws(ws) {}

Result<core::Message> WsChatService::sendMessage(
    const std::string& token,
    int64_t chatId,
    int64_t replyTo,
    const core::MessageContent& content) {
    return std::unexpected("Not implemented yet");
}

Result<SyncMessagesResponse> WsChatService::fetchAfter(
    const std::string& token,
    int64_t chatId,
    int64_t afterId,
    int limit) {
    return std::unexpected("Not implemented yet");
}

Result<SyncMessagesResponse> WsChatService::fetchBefore(
    const std::string& token,
    int64_t chatId,
    int64_t beforeId,
    int limit) {
    return std::unexpected("Not implemented yet");
}

Result<SyncMessagesResponse> WsChatService::fetchUpdated(
    const std::string& token,
    int64_t chatId,
    int64_t startId,
    int64_t endId,
    int64_t updatedAt,
    int limit) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsChatService::revokeMessage(
    const std::string& token,
    int64_t messageId) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsChatService::editMessage(
    const std::string& token,
    int64_t messageId,
    const core::MessageContent& newContent) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsChatService::markRead(
    const std::string& token,
    int64_t chatId,
    int64_t lastMessageId) {
    return std::unexpected("Not implemented yet");
}

} // namespace network
} // namespace wechat
