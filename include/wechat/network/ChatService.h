#pragma once

#include <wechat/core/Message.h>
#include <wechat/network/NetworkTypes.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat::network {

/// 消息同步响应
struct SyncMessagesResponse {
    std::vector<core::Message> messages;
    bool hasMore;
};

/// 聊天服务接口
class ChatService {
public:
    virtual ~ChatService() = default;

    /// 发送消息，返回服务端分配的完整 Message
    virtual Result<core::Message> sendMessage(
        const std::string& token,
        const std::string& chatId,
        const std::string& replyTo,
        const core::MessageContent& content) = 0;

    /// 增量同步：获取 chatId 中 timestamp > sinceTs 的消息
    virtual Result<SyncMessagesResponse> syncMessages(
        const std::string& token,
        const std::string& chatId,
        int64_t sinceTs,
        int limit) = 0;

    /// 撤回消息
    virtual VoidResult revokeMessage(
        const std::string& token,
        const std::string& messageId) = 0;

    /// 编辑消息
    virtual VoidResult editMessage(
        const std::string& token,
        const std::string& messageId,
        const core::MessageContent& newContent) = 0;

    /// 标记已读
    virtual VoidResult markRead(
        const std::string& token,
        const std::string& chatId,
        const std::string& lastMessageId) = 0;
};

} // namespace wechat::network
