#pragma once

#include <QObject>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

#include <boost/signals2/connection.hpp>
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

    void setSession(std::string const& token, std::string const& userId);
    [[nodiscard]] std::string const& currentUserId() const;

    // ── 聊天初始化 ──

    /// 初始化聊天：若已有后台同步的消息则重新推送给 UI，否则做首次增量同步
    void openChat(std::string const& chatId);

    // ── 操作（均需显式传 chatId）──

    void sendMessage(std::string const& chatId,
                     core::MessageContent const& content,
                     int64_t replyTo = 0);
    void sendTextMessage(std::string const& chatId,
                         std::string const& text);
    void loadHistory(std::string const& chatId, int limit = 20);
    void revokeMessage(int64_t messageId);
    void editMessage(int64_t messageId,
                     core::MessageContent const& newContent);

Q_SIGNALS:
    /// 新增消息（自己发的 + 别人发的，统一路径）
    void messagesInserted(QString chatId,
                          std::vector<core::Message> messages);

    /// 消息变更（撤回、编辑、已读数等）
    void messageUpdated(QString chatId, core::Message message);

    /// 消息删除
    void messageRemoved(QString chatId, int64_t messageId);

private:
    network::NetworkClient& client_;

    std::string token_;
    std::string userId_;

    struct SyncCursor {
        int64_t start = 0;
        int64_t end = 0;
    };
    std::map<std::string, SyncCursor> cursors_;

    // 网络层推送通知订阅
    boost::signals2::scoped_connection storedConn_;
    boost::signals2::scoped_connection updatedConn_;

    void onNetworkMessageStored(std::string const& chatId);
    void onNetworkMessageUpdated(std::string const& chatId,
                                  int64_t messageId);
};

} // namespace chat
} // namespace wechat
