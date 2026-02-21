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
/// 合并了原 ChatManager（业务逻辑）+ ChatController（Qt 桥接）+ ChatSignals（信号集合）。
/// 职责：
///   1. 管理会话和同步游标
///   2. 调用网络层执行操作（发送/撤回/编辑）
///   3. 订阅网络层推送通知，同步后直接发出 Qt signals
///
/// 所有数据变更必须经过网络层确认，自己发消息和收到别人消息走同一路径：
///   sendMessage() → ChatService → onMessageStored → fetchAfter → messagesInserted
class ChatPresenter : public QObject {
    Q_OBJECT

public:
    explicit ChatPresenter(network::NetworkClient& client,
                           QObject* parent = nullptr);
    ~ChatPresenter() override;

    // ── 会话 ──

    void setSession(std::string const& token, std::string const& userId);
    [[nodiscard]] std::string const& currentUserId() const;

    // ── 当前聊天 ──

    void openChat(std::string const& chatId);
    [[nodiscard]] std::string const& activeChatId() const;

    // ── 操作 ──

    void sendMessage(core::MessageContent const& content,
                     int64_t replyTo = 0);
    void sendTextMessage(std::string const& text);
    void loadHistory(int limit = 20);
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
    std::string activeChatId_;

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
