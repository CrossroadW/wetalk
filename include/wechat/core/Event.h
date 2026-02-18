#pragma once

#include <string>
#include <variant>
#include <vector>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkTypes.h>

namespace wechat::core {

/// 发送成功：服务端确认
struct MessageSentEvent {
    std::string clientTempId;  // 客户端临时 ID
    Message serverMessage;     // 服务端返回的完整消息
};

/// 发送失败
struct MessageSendFailedEvent {
    std::string clientTempId;
    network::ErrorCode code;
    std::string reason;
};

/// 收到新消息（来自同步/轮询）
struct MessagesReceivedEvent {
    std::string chatId;
    std::vector<Message> messages;
};

/// 消息被撤回
struct MessageRevokedEvent {
    std::string messageId;
    std::string chatId;
};

/// 消息被编辑
struct MessageEditedEvent {
    std::string messageId;
    std::string chatId;
    Message updatedMessage;
};

using Event = std::variant<
    std::monostate,
    MessageSentEvent,
    MessageSendFailedEvent,
    MessagesReceivedEvent,
    MessageRevokedEvent,
    MessageEditedEvent>;

} // namespace wechat::core
