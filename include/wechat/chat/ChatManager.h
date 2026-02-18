#pragma once

#include <wechat/core/EventBus.h>
#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

#include <cstdint>
#include <map>
#include <string>

namespace wechat::chat {

/// 聊天业务逻辑层（纯 C++，不依赖 Qt）
///
/// 职责：
///   - 持有 NetworkClient 引用，调用 ChatService 完成发送/同步
///   - 通过 EventBus 发布结果事件（MessageSentEvent / MessagesReceivedEvent 等）
///   - 维护增量同步游标 lastSyncTs_
///
/// 用法：
///   ChatManager mgr(networkClient, eventBus);
///   mgr.setSession(token, userId);
///   mgr.openChat(chatId);         // 切换聊天并做首次同步
///   mgr.sendTextMessage("hello");  // 发送消息
///   mgr.pollMessages();            // 轮询新消息（由外部定时器驱动）
class ChatManager {
public:
    ChatManager(network::NetworkClient &client, core::EventBus &bus);

    // ── 会话 ──

    void setSession(std::string const &token, std::string const &userId);
    [[nodiscard]] std::string const &currentUserId() const;

    // ── 当前聊天 ──

    /// 切换聊天，重置同步游标，触发首次同步
    void openChat(std::string const &chatId);
    [[nodiscard]] std::string const &activeChatId() const;

    // ── 发送 ──

    /// 发送纯文本消息，返回客户端临时 ID
    std::string sendTextMessage(std::string const &text);

    /// 发送任意内容消息，返回客户端临时 ID
    std::string sendMessage(core::MessageContent const &content,
                            std::string const &replyTo = "");

    // ── 同步（接收） ──

    /// 轮询当前聊天的新消息，有新消息时发布 MessagesReceivedEvent
    void pollMessages();

    // ── 撤回 / 编辑 ──

    void revokeMessage(std::string const &messageId);
    void editMessage(std::string const &messageId,
                     core::MessageContent const &newContent);

private:
    network::NetworkClient &client_;
    core::EventBus &bus_;

    std::string token_;
    std::string userId_;
    std::string activeChatId_;

    /// chatId -> 上次同步的时间戳
    std::map<std::string, int64_t> lastSyncTs_;

    int64_t tempIdCounter_ = 0;
    std::string generateTempId();
};

} // namespace wechat::chat
