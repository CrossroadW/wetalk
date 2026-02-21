#pragma once

#include <boost/signals2/signal.hpp>
#include <string>
#include <vector>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkTypes.h>

namespace wechat::chat {

/// Chat 模块对外的信号集合
///
/// 用于 Chat 模块和其他模块通信，使用 Boost.Signals2
/// 没有状态，只是一堆信号的聚合
class ChatSignals {
public:
    /// 消息发送成功（服务端确认）
    /// @param clientTempId 客户端临时 ID
    /// @param serverMessage 服务端返回的完整消息
    boost::signals2::signal<void(int64_t clientTempId,
                                  const core::Message& serverMessage)>
        messageSent;

    /// 消息发送失败
    /// @param clientTempId 客户端临时 ID
    /// @param code 错误码
    /// @param reason 错误原因
    boost::signals2::signal<void(int64_t clientTempId,
                                  network::ErrorCode code,
                                  const std::string& reason)>
        messageSendFailed;

    /// 收到新消息
    /// @param chatId 聊天 ID
    /// @param messages 消息列表
    boost::signals2::signal<void(const std::string& chatId,
                                  const std::vector<core::Message>& messages)>
        messagesReceived;

    /// 消息被撤回
    /// @param messageId 消息 ID
    /// @param chatId 聊天 ID
    boost::signals2::signal<void(int64_t messageId,
                                  const std::string& chatId)>
        messageRevoked;

    /// 消息被编辑
    /// @param messageId 消息 ID
    /// @param chatId 聊天 ID
    /// @param updatedMessage 更新后的消息
    boost::signals2::signal<void(int64_t messageId,
                                  const std::string& chatId,
                                  const core::Message& updatedMessage)>
        messageEdited;
};

} // namespace wechat::chat
