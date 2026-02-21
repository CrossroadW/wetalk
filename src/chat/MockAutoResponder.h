#pragma once

#include <QObject>
#include <QTimer>

#include <wechat/network/NetworkClient.h>

#include <string>

namespace wechat::chat {

/// 模拟对方用户自动发送消息
///
/// 以随机间隔（200~2000ms）持续发送消息，总数不超过 100 条。
/// 消息走完整的网络层路径，ChatPresenter 会正常感知。
/// 用于测试懒加载、upsert、滚动边界 fetch 等机制。
class MockAutoResponder : public QObject {
    Q_OBJECT

public:
    explicit MockAutoResponder(network::NetworkClient& client,
                               QObject* parent = nullptr);

    /// 设置模拟用户的会话信息
    void setResponderSession(std::string const& token,
                             std::string const& userId);

    /// 设置目标聊天并开始自动发送
    void setChatId(std::string const& chatId);

    /// 停止自动发送
    void stop();

    /// 已发送消息数
    int sentCount() const { return sentCount_; }

    /// 最大消息数（默认 100）
    void setMaxMessages(int max) { maxMessages_ = max; }

private:
    void scheduleNext();
    void sendOne();

    network::NetworkClient& client_;
    std::string token_;
    std::string userId_;
    std::string chatId_;

    QTimer timer_;
    int sentCount_ = 0;
    int maxMessages_ = 100;
};

} // namespace wechat::chat
