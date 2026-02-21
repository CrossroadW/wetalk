#pragma once

#include <wechat/network/ChatService.h>

#include <memory>
namespace wechat::network {

class MockDataStore;

class MockChatService : public ChatService {
public:
    explicit MockChatService(std::shared_ptr<MockDataStore> store);

    Result<core::Message> sendMessage(
        const std::string& token, const std::string& chatId,
        int64_t replyTo,
        const core::MessageContent& content) override;
    Result<SyncMessagesResponse> fetchAfter(
        const std::string& token, const std::string& chatId,
        int64_t afterId, int limit) override;
    Result<SyncMessagesResponse> fetchBefore(
        const std::string& token, const std::string& chatId,
        int64_t beforeId, int limit) override;
    VoidResult revokeMessage(
        const std::string& token, int64_t messageId) override;
    VoidResult editMessage(
        const std::string& token, int64_t messageId,
        const core::MessageContent& newContent) override;
    VoidResult markRead(
        const std::string& token, const std::string& chatId,
        int64_t lastMessageId) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
