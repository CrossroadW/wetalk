#pragma once

#include <QObject>
#include <QString>

#include <wechat/cache/InsertHint.h>
#include <wechat/core/Message.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wechat {
namespace network {
class ChatTransport;
} // namespace network

namespace cache {

/// 客户端消息数据层（统一入口）
///
/// Presenter 只与 MessageCache 交互，不感知网络或本地 SQLite 的存在。
///
/// 工作模式：
///   - 所有 load* 方法直接调 ChatTransport（网络，QFuture 异步）
///   - 结果在 .then() 中写入本地 SQLite，再 emit 信号通知 UI
///   - 后端推送（messagePushed / messageChangePushed）同样先落盘再通知
///
/// 对标 TDLib：MessageCache 是客户端的唯一数据入口，
/// SQLite 是主存（不是缓存），网络层是数据来源。
class MessageCache : public QObject {
    Q_OBJECT

public:
    explicit MessageCache(network::ChatTransport& transport,
                          const std::string& dbPath = ":memory:",
                          QObject* parent = nullptr);
    ~MessageCache() override;

    void setSession(std::string token, int64_t userId);

    // ── 加载操作（void 命令，结果异步经 messagesLoaded 信号返回）──

    /// 打开聊天时调用，加载最新 limit 条
    void loadLatest(int64_t chatId, int limit = 20);

    /// 触顶时调用，加载历史（firstSeq 之前）
    void loadOlder(int64_t chatId, int limit = 20);

    /// 触底时调用，加载新消息（lastSeq 之后）
    void loadNewer(int64_t chatId, int limit = 20);

    /// 引用跳转/搜索结果定位
    void jumpTo(int64_t chatId, int32_t chatSeq, int radius = 50);

    // ── 写操作（fire-and-forget，结果经推送路径回来）──

    void sendMessage(int64_t chatId, const core::MessageContent& content,
                     int64_t replyTo = 0);
    void revokeMessage(int64_t msgId);
    void editMessage(int64_t msgId, const core::MessageContent& content);

Q_SIGNALS:
    /// 批量消息已写入本地 DB，hint 指示 UI 插入方向。
    /// jumpTarget：InsertHint::Replace 时有效，UI 需滚到该 chatSeq
    void messagesLoaded(int64_t chatId,
                        std::vector<wechat::core::Message> msgs,
                        wechat::cache::InsertHint hint,
                        int32_t jumpTarget = -1);

    /// 单条消息变更（撤回/编辑/已读数）
    void messageChanged(int64_t chatId, wechat::core::Message msg);

    /// 网络失败，UI 可显示重试提示
    void loadFailed(int64_t chatId, QString reason);

private:
    void onMessagePushed(int64_t chatId, core::Message msg);
    void onMessageChangePushed(int64_t chatId, core::Message msg);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace cache
} // namespace wechat
