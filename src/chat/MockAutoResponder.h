#pragma once

#include <QObject>

#include <wechat/network/NetworkClient.h>

#include <boost/signals2/connection.hpp>
#include <string>

namespace wechat::chat {

/// 模拟对方用户自动回复
///
/// 订阅 ChatService.onMessageStored，当检测到目标聊天中有非自己发的消息时，
/// 自动以 echo 方式回复。回复走完整的网络层路径，ChatPresenter 会正常感知。
class MockAutoResponder : public QObject {
    Q_OBJECT

public:
    explicit MockAutoResponder(network::NetworkClient& client,
                               QObject* parent = nullptr);

    /// 设置模拟用户的会话信息
    void setResponderSession(std::string const& token,
                             std::string const& userId);

    /// 设置监听的目标聊天
    void setChatId(std::string const& chatId);

private:
    void onMessageStored(std::string const& chatId);

    network::NetworkClient& client_;
    std::string token_;
    std::string userId_;
    std::string chatId_;
    int64_t lastSeenId_ = 0;
    bool responding_ = false;

    boost::signals2::scoped_connection conn_;
};

} // namespace wechat::chat
