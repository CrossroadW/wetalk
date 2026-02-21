#pragma once

#include <wechat/core/Message.h>
#include <wechat/network/NetworkTypes.h>

#include <boost/signals2/signal.hpp>
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
        int64_t replyTo,
        const core::MessageContent& content) = 0;

    /// 向下同步（新消息）：获取 chatId 中 id > afterId 的消息
    /// afterId = 0 → 返回最新的 limit 条（从末尾倒数）
    /// afterId > 0 → 返回 id > afterId 的前 limit 条（升序）
    virtual Result<SyncMessagesResponse> fetchAfter(
        const std::string& token,
        const std::string& chatId,
        int64_t afterId,
        int limit) = 0;

    /// 向上同步（历史消息）：获取 chatId 中 id < beforeId 的消息
    /// beforeId = 0 → 返回最早的 limit 条（从头开始）
    /// beforeId > 0 → 返回 id < beforeId 的最后 limit 条（升序返回）
    virtual Result<SyncMessagesResponse> fetchBefore(
        const std::string& token,
        const std::string& chatId,
        int64_t beforeId,
        int limit) = 0;

    /// 增量更新：获取 chatId 中 id ∈ [startId, endId] 且 updated_at > updatedAt 的消息
    /// updatedAt = 0 → 返回所有有更新的已缓存消息
    virtual Result<SyncMessagesResponse> fetchUpdated(
        const std::string& token,
        const std::string& chatId,
        int64_t startId,
        int64_t endId,
        int64_t updatedAt,
        int limit) = 0;

    /// 撤回消息
    virtual VoidResult revokeMessage(
        const std::string& token,
        int64_t messageId) = 0;

    /// 编辑消息
    virtual VoidResult editMessage(
        const std::string& token,
        int64_t messageId,
        const core::MessageContent& newContent) = 0;

    /// 标记已读
    virtual VoidResult markRead(
        const std::string& token,
        const std::string& chatId,
        int64_t lastMessageId) = 0;

    // ── 推送通知（模拟 WebSocket）──

    /// 有新消息写入（发送/接收均触发）
    /// @param chatId 哪个聊天有变化
    boost::signals2::signal<void(const std::string& chatId)>
        onMessageStored;

    /// 消息被修改（撤回/编辑/已读数变化）
    /// @param chatId 哪个聊天有变化
    /// @param messageId 哪条消息被修改
    boost::signals2::signal<void(const std::string& chatId,
                                  int64_t messageId)>
        onMessageUpdated;
};

} // namespace wechat::network
