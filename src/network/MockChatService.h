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
        const std::string& replyTo,
        const core::MessageContent& content) override;
    Result<SyncMessagesResponse> syncMessages(
        const std::string& token, const std::string& chatId,
        int64_t sinceTs, int limit) override;
    VoidResult revokeMessage(
        const std::string& token, const std::string& messageId) override;
    VoidResult editMessage(
        const std::string& token, const std::string& messageId,
        const core::MessageContent& newContent) override;
    VoidResult markRead(
        const std::string& token, const std::string& chatId,
        const std::string& lastMessageId) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
