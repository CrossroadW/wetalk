#pragma once

#include <wechat/cache/MessageCache.h>
#include <wechat/chat/ChatPresenter.h>
#include <wechat/chat/SessionPresenter.h>
#include <wechat/core/User.h>
#include <wechat/network/NetworkClient.h>

#include <QWidget>
#include <QStackedWidget>
#include <QSplitter>

#include <map>
#include <memory>

namespace wechat {
namespace network {
class ChatTransport;
} // namespace network

namespace chat {

class ChatWidget;
class SessionListWidget;

/// 聊天页面：左侧会话列表 + 右侧聊天窗口
///
/// 组合 SessionPresenter + ChatPresenter，
/// 管理多个 ChatWidget（按 chatId 切换）。
///
/// 内部持有 ChatTransport + MessageCache 的所有权。
/// 若 NetworkClient::ws() 返回 nullptr（Mock 模式），消息功能不可用。
class ChatPage : public QWidget {
    Q_OBJECT

public:
    explicit ChatPage(network::NetworkClient& client,
                      QWidget* parent = nullptr);

    void setSession(const std::string& token, int64_t userId);

    /// 外部请求打开某个聊天（例如从通讯录点击好友）
    void openChat(int64_t chatId, const core::User& peer);

    ChatPresenter* chatPresenter() { return chatPresenter_.get(); }
    SessionPresenter* sessionPresenter() { return sessionPresenter_.get(); }

private Q_SLOTS:
    void onSessionSelected(int64_t chatId);

private:
    void setupUI();
    ChatWidget* getOrCreateChatWidget(int64_t chatId);

    network::NetworkClient& client_;
    std::string token_;
    int64_t userId_ = 0;

    // 按顺序声明：transport → cache → presenter（析构反序）
    std::unique_ptr<network::ChatTransport> chatTransport_;
    std::unique_ptr<cache::MessageCache>    messageCache_;
    std::unique_ptr<ChatPresenter>          chatPresenter_;
    std::unique_ptr<SessionPresenter>       sessionPresenter_;

    SessionListWidget* sessionList_ = nullptr;
    QStackedWidget*    chatStack_   = nullptr;
    QWidget*           placeholder_ = nullptr;

    std::map<int64_t, ChatWidget*> chatWidgets_;
    std::map<int64_t, core::User>  peers_;
};

} // namespace chat
} // namespace wechat
