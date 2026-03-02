#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include <wechat/contacts/ContactsPresenter.h>
#include <wechat/core/User.h>
#include <wechat/network/NetworkClient.h>

using namespace wechat;

Q_DECLARE_METATYPE(std::vector<core::User>)
Q_DECLARE_METATYPE(core::User)

class ContactsPresenterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_contacts_presenter";
            static char* argv[] = {arg0};
            static QCoreApplication app(argc, argv);
        }
        qRegisterMetaType<std::vector<core::User>>();
        qRegisterMetaType<core::User>();
    }

    void SetUp() override {
        client = network::createMockClient();

        auto regA = client->auth().registerUser("alice", "p");
        auto regB = client->auth().registerUser("bob", "p");
        auto regC = client->auth().registerUser("carol", "p");

        tokenA = regA->token;
        aliceId = regA->id;
        bobId = regB->id;
        carolId = regC->id;

        presenter = std::make_unique<contacts::ContactsPresenter>(*client);
        presenter->setSession(tokenA, aliceId);
    }

    std::unique_ptr<network::NetworkClient> client;
    std::unique_ptr<contacts::ContactsPresenter> presenter;
    std::string tokenA;
    int64_t aliceId = 0, bobId = 0, carolId = 0;
};

// ═══════════════════════════════════════════════════════════════
// 好友列表测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ContactsPresenterTest, LoadFriendsEmpty) {
    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::friendsLoaded);
    ASSERT_TRUE(spy.isValid());

    presenter->loadFriends();

    ASSERT_EQ(spy.count(), 1);
    auto friends = spy.at(0).at(0).value<std::vector<core::User>>();
    EXPECT_EQ(friends.size(), 0u);
}

TEST_F(ContactsPresenterTest, LoadFriendsAfterAdd) {
    client->contacts().addFriend(tokenA, bobId);

    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::friendsLoaded);

    presenter->loadFriends();

    ASSERT_EQ(spy.count(), 1);
    auto friends = spy.at(0).at(0).value<std::vector<core::User>>();
    ASSERT_EQ(friends.size(), 1u);
    EXPECT_EQ(friends[0].id, bobId);
    EXPECT_EQ(friends[0].username, "bob");
}

// ═══════════════════════════════════════════════════════════════
// 添加好友测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ContactsPresenterTest, AddFriendSuccess) {
    QSignalSpy addedSpy(presenter.get(), &contacts::ContactsPresenter::friendAdded);
    QSignalSpy errorSpy(presenter.get(), &contacts::ContactsPresenter::errorOccurred);

    presenter->addFriend(bobId);

    ASSERT_EQ(addedSpy.count(), 1);
    EXPECT_EQ(errorSpy.count(), 0);

    auto user = addedSpy.at(0).at(0).value<core::User>();
    EXPECT_EQ(user.id, bobId);
    EXPECT_EQ(user.username, "bob");
}

TEST_F(ContactsPresenterTest, AddFriendSelf) {
    QSignalSpy addedSpy(presenter.get(), &contacts::ContactsPresenter::friendAdded);
    QSignalSpy errorSpy(presenter.get(), &contacts::ContactsPresenter::errorOccurred);

    presenter->addFriend(aliceId);

    EXPECT_EQ(addedSpy.count(), 0);
    ASSERT_EQ(errorSpy.count(), 1);
}

// ═══════════════════════════════════════════════════════════════
// 删除好友测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ContactsPresenterTest, RemoveFriendSuccess) {
    client->contacts().addFriend(tokenA, bobId);

    QSignalSpy removedSpy(presenter.get(), &contacts::ContactsPresenter::friendRemoved);

    presenter->removeFriend(bobId);

    ASSERT_EQ(removedSpy.count(), 1);
    auto removedId = removedSpy.at(0).at(0).toLongLong();
    EXPECT_EQ(removedId, bobId);

    // 确认好友列表为空
    QSignalSpy listSpy(presenter.get(), &contacts::ContactsPresenter::friendsLoaded);
    presenter->loadFriends();
    auto friends = listSpy.at(0).at(0).value<std::vector<core::User>>();
    EXPECT_EQ(friends.size(), 0u);
}

// ═══════════════════════════════════════════════════════════════
// 搜索测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ContactsPresenterTest, SearchUserSuccess) {
    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::searchResults);

    presenter->searchUser("bob");

    ASSERT_EQ(spy.count(), 1);
    auto results = spy.at(0).at(0).value<std::vector<core::User>>();
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].username, "bob");
}

TEST_F(ContactsPresenterTest, SearchUserMultipleResults) {
    // 注册名字相似的用户
    client->auth().registerUser("bobby", "p");

    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::searchResults);

    presenter->searchUser("bob");

    ASSERT_EQ(spy.count(), 1);
    auto results = spy.at(0).at(0).value<std::vector<core::User>>();
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(ContactsPresenterTest, SearchUserNoResults) {
    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::searchResults);

    presenter->searchUser("nonexistent");

    ASSERT_EQ(spy.count(), 1);
    auto results = spy.at(0).at(0).value<std::vector<core::User>>();
    EXPECT_EQ(results.size(), 0u);
}

// ═══════════════════════════════════════════════════════════════
// 完整流程测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ContactsPresenterTest, AddThenLoadFriends) {
    presenter->addFriend(bobId);
    presenter->addFriend(carolId);

    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::friendsLoaded);
    presenter->loadFriends();

    ASSERT_EQ(spy.count(), 1);
    auto friends = spy.at(0).at(0).value<std::vector<core::User>>();
    EXPECT_EQ(friends.size(), 2u);
}

TEST_F(ContactsPresenterTest, AddRemoveAddFriend) {
    presenter->addFriend(bobId);
    presenter->removeFriend(bobId);

    QSignalSpy spy(presenter.get(), &contacts::ContactsPresenter::friendsLoaded);
    presenter->loadFriends();

    auto friends = spy.at(0).at(0).value<std::vector<core::User>>();
    EXPECT_EQ(friends.size(), 0u);

    // 重新添加
    QSignalSpy addedSpy(presenter.get(), &contacts::ContactsPresenter::friendAdded);
    presenter->addFriend(bobId);
    ASSERT_EQ(addedSpy.count(), 1);
}
