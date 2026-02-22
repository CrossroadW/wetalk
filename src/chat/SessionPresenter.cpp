#include <wechat/chat/SessionPresenter.h>

#include <wechat/core/Message.h>
#include <wechat/network/ChatService.h>

#include <algorithm>

namespace wechat {
namespace chat {

SessionPresenter::SessionPresenter(network::NetworkClient& client,
                                   QObject* parent)
    : QObject(parent), client(client) {}

void SessionPresenter::setSession(const std::string& token, int64_t userId) {
    this->token = token;
    this->userId = userId;

    // 监听新消息通知，实时更新会话列表
    connect(&client.chat(), &network::ChatService::messageStored,
            this, &SessionPresenter::onMessageStored);
}

void SessionPresenter::loadSessions() {
    auto groups = client.groups().listMyGroups(token);
    if (!groups.has_value()) return;

    std::vector<SessionItem> sessions;
    for (auto& group : groups.value()) {
        auto item = buildSessionItem(group.id);
        if (item.chatId != 0) {
            sessions.push_back(std::move(item));
        }
    }

    // 按最后消息时间倒序排列
    std::sort(sessions.begin(), sessions.end(),
              [](auto& a, auto& b) {
                  return a.lastTimestamp > b.lastTimestamp;
              });

    Q_EMIT sessionsLoaded(sessions);
}

void SessionPresenter::onMessageStored(int64_t chatId) {
    if (token.empty()) return;

    auto item = buildSessionItem(chatId);
    if (item.chatId != 0) {
        Q_EMIT sessionUpdated(item);
    }
}

SessionItem SessionPresenter::buildSessionItem(int64_t chatId) {
    SessionItem item;
    item.chatId = chatId;

    // 获取群成员构建名字
    auto members = client.groups().listMembers(token, chatId);
    if (members.has_value()) {
        for (auto memberId : members.value()) {
            if (memberId != userId) {
                auto user = client.auth().getCurrentUser(token);
                // 获取对方用户名需要通过搜索或好友列表
                auto friends = client.contacts().listFriends(token);
                if (friends.has_value()) {
                    for (auto& f : friends.value()) {
                        if (f.id == memberId) {
                            item.displayName = f.username;
                            break;
                        }
                    }
                }
                if (item.displayName.empty()) {
                    item.displayName = "User " + std::to_string(memberId);
                }
                break;
            }
        }
        // 多人群聊
        if (members.value().size() > 2) {
            item.displayName = "Group (" +
                std::to_string(members.value().size()) + ")";
        }
    }

    // 获取最后一条消息
    auto sync = client.chat().fetchAfter(token, chatId, 0, 1);
    if (sync.has_value() && !sync.value().messages.empty()) {
        auto& msg = sync.value().messages.back();
        item.lastTimestamp = msg.timestamp;
        if (!msg.content.empty()) {
            if (auto* text = std::get_if<core::TextContent>(&msg.content[0])) {
                item.lastMessage = text->text;
            } else if (auto* res = std::get_if<core::ResourceContent>(&msg.content[0])) {
                if (res->type == core::ResourceType::Image) {
                    item.lastMessage = "[Image]";
                } else {
                    item.lastMessage = "[File]";
                }
            } else {
                item.lastMessage = "[Message]";
            }
        }
        if (msg.revoked) {
            item.lastMessage = "[Revoked]";
        }
    }

    return item;
}

} // namespace chat
} // namespace wechat
