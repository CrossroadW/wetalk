#pragma once

#include <QObject>

#include <wechat/cache/InsertHint.h>
#include <wechat/cache/MessageCache.h>
#include <wechat/core/Message.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat {
namespace chat {

/// 聊天模块的 Presenter（MVP 中唯一的中间层）
///
/// 职责：
///   1. 接收 ChatWidget 的用户意图，转发给 MessageCache
///   2. 将 MessageCache 信号转发给 ChatWidget
///   3. 持有 currentUserId（UI 区分自己/对方消息用）
///
/// 不持有"当前聊天"概念，所有操作显式传 chatId。
/// 多个 ChatWidget 可共享同一个 Presenter，各自按 chatId 过滤信号。
class ChatPresenter : public QObject {
    Q_OBJECT

public:
    explicit ChatPresenter(cache::MessageCache& cache,
                           QObject* parent = nullptr);
    ~ChatPresenter() override;

    void setSession(std::string const& token, int64_t userId);
    [[nodiscard]] int64_t currentUserId() const;

    // ── 加载（异步，结果经 messagesLoaded 信号返回）──

    void loadLatest(int64_t chatId, int limit = 20);
    void loadOlder(int64_t chatId, int limit = 20);
    void loadNewer(int64_t chatId, int limit = 20);
    void jumpTo(int64_t chatId, int32_t chatSeq, int radius = 50);

    // ── 写操作（fire-and-forget，结果经推送路径返回）──

    void sendMessage(int64_t chatId,
                     core::MessageContent const& content,
                     int64_t replyTo = 0);
    void sendTextMessage(int64_t chatId, std::string const& text);
    void revokeMessage(int64_t msgId);
    void editMessage(int64_t msgId, core::MessageContent const& content);

Q_SIGNALS:
    /// 批量消息已写入本地 DB，hint 指示 UI 插入方向
    void messagesLoaded(int64_t chatId,
                        std::vector<wechat::core::Message> msgs,
                        wechat::cache::InsertHint hint,
                        int32_t jumpTarget = -1);

    /// 单条消息变更（撤回/编辑/已读数）
    void messageChanged(int64_t chatId, wechat::core::Message msg);

    /// 网络失败
    void loadFailed(int64_t chatId, QString reason);

private:
    cache::MessageCache& cache_;
    int64_t userId_ = 0;
};

} // namespace chat
} // namespace wechat
