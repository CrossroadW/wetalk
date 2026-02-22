#include <wechat/contacts/ContactsPresenter.h>

namespace wechat {
namespace contacts {

ContactsPresenter::ContactsPresenter(network::NetworkClient& client,
                                     QObject* parent)
    : QObject(parent), client(client) {}

void ContactsPresenter::setSession(const std::string& token, int64_t userId) {
    this->token = token;
    this->userId = userId;
}

void ContactsPresenter::loadFriends() {
    auto result = client.contacts().listFriends(token);
    if (result.has_value()) {
        Q_EMIT friendsLoaded(result.value());
    } else {
        Q_EMIT errorOccurred(QString::fromStdString(result.error()));
    }
}

void ContactsPresenter::searchUser(const std::string& keyword) {
    auto result = client.contacts().searchUser(token, keyword);
    if (result.has_value()) {
        Q_EMIT searchResults(result.value());
    } else {
        Q_EMIT errorOccurred(QString::fromStdString(result.error()));
    }
}

void ContactsPresenter::addFriend(int64_t targetUserId) {
    auto result = client.contacts().addFriend(token, targetUserId);
    if (result.has_value()) {
        // 添加成功后获取好友信息返回
        auto friends = client.contacts().listFriends(token);
        if (friends.has_value()) {
            for (auto& f : friends.value()) {
                if (f.id == targetUserId) {
                    Q_EMIT friendAdded(f);
                    return;
                }
            }
        }
        // 如果无法获取好友信息，返回最小 User
        Q_EMIT friendAdded(core::User{targetUserId});
    } else {
        Q_EMIT errorOccurred(QString::fromStdString(result.error()));
    }
}

void ContactsPresenter::removeFriend(int64_t targetUserId) {
    auto result = client.contacts().removeFriend(token, targetUserId);
    if (result.has_value()) {
        Q_EMIT friendRemoved(targetUserId);
    } else {
        Q_EMIT errorOccurred(QString::fromStdString(result.error()));
    }
}

} // namespace contacts
} // namespace wechat
