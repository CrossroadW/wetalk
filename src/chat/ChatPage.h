#pragma once

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
namespace chat {

class ChatWidget;
class SessionListWidget;

/// 聊天页面：左侧会话列表 + 右侧聊天窗口
///
/// 组合 SessionPresenter + ChatPresenter，
/// 管理多个 ChatWidget（按 chatId 切换）。
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

    network::NetworkClient& client;
    std::string token;
    int64_t userId = 0;

    std::unique_ptr<ChatPresenter> chatPresenter_;
    std::unique_ptr<SessionPresenter> sessionPresenter_;

    SessionListWidget* sessionList = nullptr;
    QStackedWidget* chatStack = nullptr;
    QWidget* placeholder = nullptr;

    std::map<int64_t, ChatWidget*> chatWidgets;
    // chatId → peer info (for display)
    std::map<int64_t, core::User> peers;
};

} // namespace chat
} // namespace wechat
