#pragma once

#include <QObject>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

#include <string>

namespace wechat::chat {

/// 模拟对方用户发送消息
///
/// 通过 NetworkClient 以另一个用户的身份发送消息。
/// 消息写入后 ChatService 自动触发 onMessageStored 通知，
/// ChatPresenter 会通过同步机制感知到新消息。
class MockAutoResponder : public QObject {
    Q_OBJECT

public:
    explicit MockAutoResponder(network::NetworkClient& client,
                               QObject* parent = nullptr);

    /// 设置模拟用户的会话信息
    void setResponderSession(std::string const& token,
                             std::string const& userId);

    /// 设置回复的目标聊天
    void setChatId(std::string const& chatId);

    /// 立即以模拟用户身份发送一条消息
    void sendMessage(std::string const& text);

private:
    network::NetworkClient& client_;
    std::string token_;
    std::string userId_;
    std::string chatId_;
};

} // namespace wechat::chat
