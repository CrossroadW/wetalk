#pragma once

#include <wechat/core/User.h>

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

#include <vector>

namespace wechat {
namespace contacts {

class ContactsPresenter;

class ContactsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ContactsWidget(QWidget* parent = nullptr);

    void setPresenter(ContactsPresenter* presenter);

Q_SIGNALS:
    void friendSelected(core::User user);

private Q_SLOTS:
    void onSearch();
    void onFriendsLoaded(std::vector<core::User> friends);
    void onSearchResults(std::vector<core::User> users);
    void onFriendAdded(core::User user);
    void onFriendRemoved(int64_t userId);
    void onError(QString message);

private:
    void setupUI();
    void setupConnections();

    QLineEdit* searchInput = nullptr;
    QPushButton* searchButton = nullptr;
    QListWidget* friendList = nullptr;
    QListWidget* searchResultList = nullptr;
    QLabel* statusLabel = nullptr;

    ContactsPresenter* presenter = nullptr;
    std::vector<core::User> friends;
};

} // namespace contacts
} // namespace wechat
