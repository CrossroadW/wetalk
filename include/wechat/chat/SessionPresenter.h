#pragma once

#include <wechat/core/User.h>
#include <wechat/network/NetworkClient.h>

#include <QObject>
#include <QString>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat {
namespace chat {

/// 会话条目（纯数据）
struct SessionItem {
    int64_t chatId = 0;
    std::string displayName;
    std::string lastMessage;
    int64_t lastTimestamp = 0;
};

/// 会话列表 Presenter
///
/// 从 GroupService 拉取所有群组，
/// 从 ChatService 拉取每个群的最后一条消息，
/// 监听 ChatService::messageStored 实时更新。
class SessionPresenter : public QObject {
    Q_OBJECT

public:
    explicit SessionPresenter(network::NetworkClient& client,
                              QObject* parent = nullptr);

    void setSession(const std::string& token, int64_t userId);
    void loadSessions();

Q_SIGNALS:
    void sessionsLoaded(std::vector<SessionItem> sessions);
    void sessionUpdated(SessionItem session);

private:
    void onMessageStored(int64_t chatId);
    SessionItem buildSessionItem(int64_t chatId);

    network::NetworkClient& client;
    std::string token;
    int64_t userId = 0;
};

} // namespace chat
} // namespace wechat
