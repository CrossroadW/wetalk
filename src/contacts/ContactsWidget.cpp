#include "ContactsWidget.h"

#include <wechat/contacts/ContactsPresenter.h>

#include <QVBoxLayout>
#include <QHBoxLayout>

namespace wechat {
namespace contacts {

ContactsWidget::ContactsWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ContactsWidget::setPresenter(ContactsPresenter* presenter) {
    this->presenter = presenter;
    setupConnections();
    presenter->loadFriends();
}

void ContactsWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // 搜索栏
    auto* searchLayout = new QHBoxLayout();
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Search user...");
    searchButton = new QPushButton("Search");
    searchLayout->addWidget(searchInput);
    searchLayout->addWidget(searchButton);
    layout->addLayout(searchLayout);

    // 搜索结果列表（默认隐藏）
    searchResultList = new QListWidget;
    searchResultList->setVisible(false);
    searchResultList->setMaximumHeight(150);
    layout->addWidget(searchResultList);

    // 好友列表标题
    auto* friendLabel = new QLabel("Friends");
    friendLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(friendLabel);

    // 好友列表
    friendList = new QListWidget;
    layout->addWidget(friendList);

    // 状态栏
    statusLabel = new QLabel;
    statusLabel->setStyleSheet("color: #999;");
    layout->addWidget(statusLabel);
}

void ContactsWidget::setupConnections() {
    connect(searchButton, &QPushButton::clicked, this, &ContactsWidget::onSearch);
    connect(searchInput, &QLineEdit::returnPressed, this, &ContactsWidget::onSearch);

    // 双击好友列表 → 选中好友
    connect(friendList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                auto idx = friendList->row(item);
                if (idx >= 0 && idx < static_cast<int>(friends.size())) {
                    Q_EMIT friendSelected(friends[idx]);
                }
            });

    // 双击搜索结果 → 添加好友
    connect(searchResultList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                auto userId = item->data(Qt::UserRole).toLongLong();
                if (presenter) {
                    presenter->addFriend(userId);
                }
            });

    if (presenter) {
        connect(presenter, &ContactsPresenter::friendsLoaded,
                this, &ContactsWidget::onFriendsLoaded);
        connect(presenter, &ContactsPresenter::searchResults,
                this, &ContactsWidget::onSearchResults);
        connect(presenter, &ContactsPresenter::friendAdded,
                this, &ContactsWidget::onFriendAdded);
        connect(presenter, &ContactsPresenter::friendRemoved,
                this, &ContactsWidget::onFriendRemoved);
        connect(presenter, &ContactsPresenter::errorOccurred,
                this, &ContactsWidget::onError);
    }
}

void ContactsWidget::onSearch() {
    auto keyword = searchInput->text().toStdString();
    if (keyword.empty() || !presenter) return;
    presenter->searchUser(keyword);
}

void ContactsWidget::onFriendsLoaded(std::vector<core::User> friends) {
    this->friends = friends;
    friendList->clear();
    for (auto& f : friends) {
        friendList->addItem(QString::fromStdString(f.username));
    }
    statusLabel->setText(
        QString("%1 friend(s)").arg(friends.size()));
}

void ContactsWidget::onSearchResults(std::vector<core::User> users) {
    searchResultList->clear();
    if (users.empty()) {
        searchResultList->setVisible(false);
        statusLabel->setText("No users found");
        return;
    }
    searchResultList->setVisible(true);
    for (auto& u : users) {
        auto* item = new QListWidgetItem(
            QString::fromStdString(u.username) + " (double-click to add)");
        item->setData(Qt::UserRole, static_cast<qlonglong>(u.id));
        searchResultList->addItem(item);
    }
    statusLabel->setText(
        QString("Found %1 user(s)").arg(users.size()));
}

void ContactsWidget::onFriendAdded(core::User user) {
    friends.push_back(user);
    friendList->addItem(QString::fromStdString(user.username));
    searchResultList->setVisible(false);
    searchInput->clear();
    statusLabel->setText(
        QString("Added %1").arg(QString::fromStdString(user.username)));
}

void ContactsWidget::onFriendRemoved(int64_t userId) {
    for (auto it = friends.begin(); it != friends.end(); ++it) {
        if (it->id == userId) {
            auto row = static_cast<int>(std::distance(friends.begin(), it));
            delete friendList->takeItem(row);
            friends.erase(it);
            break;
        }
    }
    statusLabel->setText(
        QString("%1 friend(s)").arg(friends.size()));
}

void ContactsWidget::onError(QString message) {
    statusLabel->setText(message);
    statusLabel->setStyleSheet("color: red;");
}

} // namespace contacts
} // namespace wechat
