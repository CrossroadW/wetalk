#pragma once

#include <QFuture>
#include <QObject>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkTypes.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat {
namespace network {

struct SyncResponse {
    std::vector<core::Message> messages;
    bool hasMore = false;
};

/// 聊天传输层接口（纯协议，无状态，全异步）
///
/// 读操作返回 QFuture<Result<SyncResponse>>，通过 .then() 链式处理。
/// 写操作 fire-and-forget（void），结果经后端推送信号回来。
///
/// 分页游标使用 chatSeq（per-chat 单调自增序号），不用全局 id。
class ChatTransport : public QObject {
    Q_OBJECT

public:
    explicit ChatTransport(QObject* parent = nullptr) : QObject(parent) {}
    ~ChatTransport() override = default;

    // ── 读操作（全异步）──

    /// 加载最新 limit 条消息（打开聊天时调用）
    virtual QFuture<Result<SyncResponse>> fetchLatest(
        const std::string& token, int64_t chatId, int limit) = 0;

    /// 加载 chatSeq 之前的 limit 条（触顶加载历史）
    /// chatSeq = 0 → 从最早开始
    virtual QFuture<Result<SyncResponse>> fetchBefore(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int limit) = 0;

    /// 加载 chatSeq 之后的 limit 条（触底 / 实时追赶）
    virtual QFuture<Result<SyncResponse>> fetchAfter(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int limit) = 0;

    /// 加载 chatSeq 附近的消息（jumpTo 引用跳转）
    virtual QFuture<Result<SyncResponse>> fetchAround(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int radius) = 0;

    /// 增量更新：返回 updatedAt > maxUpdatedAt 且在已缓存区间内的消息
    virtual QFuture<Result<SyncResponse>> fetchUpdated(
        const std::string& token, int64_t chatId,
        int64_t maxUpdatedAt, int limit) = 0;

    // ── 写操作（fire-and-forget）──

    /// 发送消息。结果经后端推送 messagePushed 信号回来。
    virtual void sendMessage(const std::string& token, int64_t chatId,
                             int64_t replyTo,
                             const core::MessageContent& content) = 0;

    virtual void revokeMessage(const std::string& token, int64_t msgId) = 0;

    virtual void editMessage(const std::string& token, int64_t msgId,
                             const core::MessageContent& content) = 0;

Q_SIGNALS:
    /// 后端推送：新消息到达（自己发的 + 别人发的走同一路径）
    void messagePushed(int64_t chatId, wechat::core::Message msg);

    /// 后端推送：消息被撤回/编辑/已读数变化
    void messageChangePushed(int64_t chatId, wechat::core::Message msg);
};

} // namespace network
} // namespace wechat
