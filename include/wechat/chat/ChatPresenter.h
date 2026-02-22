#pragma once

#include <QObject>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wechat {
namespace chat {

/// 聊天模块的 Presenter（MVP 中唯一的中间层）
///
/// 职责：
///   1. 管理同步游标（每个聊天独立）
///   2. 调用网络层执行操作（发送/撤回/编辑）
///   3. 订阅网络层推送通知，同步后直接发出 Qt signals
///
/// 不持有"当前聊天"概念，所有操作显式传 chatId。
/// 多个 ChatWidget 共享同一个 Presenter，各自按 chatId 过滤信号。
///
/// 发送路径：
///   sendMessage(chatId) → ChatService → onMessageStored → fetchAfter → messagesInserted
class ChatPresenter : public QObject {
    Q_OBJECT

public:
    explicit ChatPresenter(network::NetworkClient& client,
                           QObject* parent = nullptr);
    ~ChatPresenter() override;

    // ── 会话 ──

    void setSession(std::string const& token, int64_t userId);
    [[nodiscard]] int64_t currentUserId() const;

    // ── 聊天初始化 ──

    /// 确保 chatId 的同步游标存在（不 fetch、不 emit）
    void openChat(int64_t chatId);

    // ── 操作（均需显式传 chatId）──

    void sendMessage(int64_t chatId,
                     core::MessageContent const& content,
                     int64_t replyTo = 0);
    void sendTextMessage(int64_t chatId,
                         std::string const& text);
    /// 打开聊天时调用，fetchAfter(0) 加载最新消息
    void loadLatest(int64_t chatId, int limit = 20);
    /// 向上翻页，fetchBefore(start) 加载历史消息
    void loadHistory(int64_t chatId, int limit = 20);
    void revokeMessage(int64_t messageId);
    void editMessage(int64_t messageId,
                     core::MessageContent const& newContent);

Q_SIGNALS:
    /// 新增消息（自己发的 + 别人发的，统一路径）
    void messagesInserted(int64_t chatId,
                          std::vector<core::Message> messages);

    /// 消息变更（撤回、编辑、已读数等）
    void messageUpdated(int64_t chatId, core::Message message);

    /// 消息删除
    void messageRemoved(int64_t chatId, int64_t messageId);

private:
    network::NetworkClient& client_;

    std::string token_;
    int64_t userId_ = 0;

    struct SyncCursor {
        int64_t start = 0;
        int64_t end = 0;
        int64_t maxUpdatedAt = 0;
    };
    std::map<int64_t, SyncCursor> cursors_;

    void onNetworkMessageStored(int64_t chatId);
    void onNetworkMessageUpdated(int64_t chatId, int64_t messageId);
    void syncUpdated(int64_t chatId);
    void updateMaxUpdatedAt(SyncCursor& cursor,
                            std::vector<core::Message> const& msgs);
};

} // namespace chat
} // namespace wechat
