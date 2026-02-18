#pragma once

#include <QObject>
#include <QTimer>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

#include <string>
#include <vector>

namespace wechat::chat {

/// 模拟对方用户发送消息
///
/// 通过 NetworkClient 以另一个用户的身份直接发送消息到 MockDataStore。
/// ChatManager 的 pollMessages() 会自然发现这些新消息。
class MockAutoResponder : public QObject {
    Q_OBJECT

public:
    explicit MockAutoResponder(network::NetworkClient &client,
                               QObject *parent = nullptr);

    /// 设置模拟用户的会话信息
    void setResponderSession(std::string const &token,
                             std::string const &userId);

    /// 设置回复的目标聊天
    void setChatId(std::string const &chatId);

    /// 安排一条延迟消息
    void scheduleMessage(std::string const &text, int delayMs);

    void stop();

private:
    network::NetworkClient &client_;
    std::string token_;
    std::string userId_;
    std::string chatId_;
};

} // namespace wechat::chat
