#pragma once

#include <wechat/chat/ChatSignals.h>
#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace wechat::chat {

/// 聊天业务逻辑层（纯 C++，不依赖 Qt）
///
/// 职责：
///   - 持有 NetworkClient 引用，调用 ChatService 完成发送/同步
///   - 通过注入的 ChatSignals 触发信号（messageSent / messagesReceived 等）
///   - 维护增量同步游标 SyncCursor（start/end 双向区间）
///
/// 用法：
///   auto chatSignals = std::make_shared<ChatSignals>();
///   ChatManager mgr(networkClient, chatSignals);
///   mgr.setSession(token, userId);
///   mgr.openChat(chatId);         // 切换聊天并做首次同步
///   mgr.sendTextMessage("hello");  // 发送消息
///   mgr.pollMessages();            // 轮询新消息（由外部定时器驱动）
///   mgr.loadHistory();             // 加载历史消息（向上翻页）
class ChatManager {
public:
    ChatManager(network::NetworkClient &client,
                std::shared_ptr<ChatSignals> chatSignals);

    // ── 会话 ──

    void setSession(std::string const &token, std::string const &userId);
    [[nodiscard]] std::string const &currentUserId() const;

    // ── 当前聊天 ──

    /// 切换聊天，重置同步游标，触发首次同步
    void openChat(std::string const &chatId);
    [[nodiscard]] std::string const &activeChatId() const;

    // ── 发送 ──

    /// 发送纯文本消息，返回客户端临时 ID
    int64_t sendTextMessage(std::string const &text);

    /// 发送任意内容消息，返回客户端临时 ID
    int64_t sendMessage(core::MessageContent const &content,
                        int64_t replyTo = 0);

    // ── 同步（接收） ──

    /// 轮询当前聊天的新消息（向下，after_id = end）
    void pollMessages();

    /// 加载历史消息（向上，before_id = start）
    void loadHistory(int limit = 20);

    // ── 撤回 / 编辑 ──

    void revokeMessage(int64_t messageId);
    void editMessage(int64_t messageId,
                     core::MessageContent const &newContent);

private:
    network::NetworkClient &client_;
    std::shared_ptr<ChatSignals> signals_;

    std::string token_;
    std::string userId_;
    std::string activeChatId_;

    /// 缓存区间游标：[start, end]
    /// start = 已加载的最小消息 ID（向上加载时更新）
    /// end   = 已加载的最大消息 ID（向下轮询时更新）
    struct SyncCursor {
        int64_t start = 0;  // before_id 方向的游标
        int64_t end = 0;    // after_id 方向的游标
    };

    /// chatId -> 同步游标
    std::map<std::string, SyncCursor> cursors_;

    int64_t tempIdCounter_ = 0;
    int64_t generateTempId();
};

} // namespace wechat::chat
