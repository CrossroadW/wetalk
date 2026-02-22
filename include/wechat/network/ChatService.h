#pragma once

#include <QObject>

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
class ChatService : public QObject {
    Q_OBJECT

public:
    explicit ChatService(QObject* parent = nullptr) : QObject(parent) {}
    ~ChatService() override = default;

    /// 发送消息，返回服务端分配的完整 Message
    virtual Result<core::Message> sendMessage(
        const std::string& token,
        int64_t chatId,
        int64_t replyTo,
        const core::MessageContent& content) = 0;

    /// 向下同步（新消息）：获取 chatId 中 id > afterId 的消息
    /// afterId = 0 → 返回最新的 limit 条（从末尾倒数）
    /// afterId > 0 → 返回 id > afterId 的前 limit 条（升序）
    virtual Result<SyncMessagesResponse> fetchAfter(
        const std::string& token,
        int64_t chatId,
        int64_t afterId,
        int limit) = 0;

    /// 向上同步（历史消息）：获取 chatId 中 id < beforeId 的消息
    /// beforeId = 0 → 返回最早的 limit 条（从头开始）
    /// beforeId > 0 → 返回 id < beforeId 的最后 limit 条（升序返回）
    virtual Result<SyncMessagesResponse> fetchBefore(
        const std::string& token,
        int64_t chatId,
        int64_t beforeId,
        int limit) = 0;

    /// 增量更新：获取 chatId 中 id ∈ [startId, endId] 且 updated_at > updatedAt 的消息
    /// updatedAt = 0 → 返回所有有更新的已缓存消息
    virtual Result<SyncMessagesResponse> fetchUpdated(
        const std::string& token,
        int64_t chatId,
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
        int64_t chatId,
        int64_t lastMessageId) = 0;

Q_SIGNALS:
    /// 有新消息写入（发送/接收均触发）
    void messageStored(int64_t chatId);

    /// 消息被修改（撤回/编辑/已读数变化）
    void messageUpdated(int64_t chatId, int64_t messageId);
};

} // namespace wechat::network
