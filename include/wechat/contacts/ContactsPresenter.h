#pragma once

#include <wechat/core/User.h>
#include <wechat/network/NetworkClient.h>

#include <QObject>
#include <QString>

#include <string>
#include <vector>

namespace wechat {
namespace contacts {

class ContactsPresenter : public QObject {
    Q_OBJECT

public:
    explicit ContactsPresenter(network::NetworkClient& client,
                               QObject* parent = nullptr);

    void setSession(const std::string& token, int64_t userId);

    void loadFriends();
    void searchUser(const std::string& keyword);
    void addFriend(int64_t targetUserId);
    void removeFriend(int64_t targetUserId);

Q_SIGNALS:
    void friendsLoaded(std::vector<core::User> friends);
    void searchResults(std::vector<core::User> users);
    void friendAdded(core::User user);
    void friendRemoved(int64_t userId);
    void errorOccurred(QString message);

private:
    network::NetworkClient& client;
    std::string token;
    int64_t userId = 0;
};

} // namespace contacts
} // namespace wechat
