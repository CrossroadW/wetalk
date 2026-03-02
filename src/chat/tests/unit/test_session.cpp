#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include <wechat/chat/SessionPresenter.h>
#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>

using namespace wechat;

Q_DECLARE_METATYPE(std::vector<chat::SessionItem>)
Q_DECLARE_METATYPE(chat::SessionItem)

class SessionPresenterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_session";
            static char* argv[] = {arg0};
            static QCoreApplication app(argc, argv);
        }
        qRegisterMetaType<std::vector<chat::SessionItem>>();
        qRegisterMetaType<chat::SessionItem>();
    }

    void SetUp() override {
        client = network::createMockClient();

        auto regA = client->auth().registerUser("alice", "p");
        auto regB = client->auth().registerUser("bob", "p");
        tokenA = regA->token;
        tokenB = regB->token;
        aliceId = regA->id;
        bobId = regB->id;

        client->contacts().addFriend(tokenA, bobId);

        presenter = std::make_unique<chat::SessionPresenter>(*client);
    }

    std::unique_ptr<network::NetworkClient> client;
    std::unique_ptr<chat::SessionPresenter> presenter;
    std::string tokenA, tokenB;
    int64_t aliceId = 0, bobId = 0;
};

// ═══════════════════════════════════════════════════════════════
// 加载会话列表测试
// ═══════════════════════════════════════════════════════════════

TEST_F(SessionPresenterTest, LoadSessionsEmpty) {
    presenter->setSession(tokenA, aliceId);

    QSignalSpy spy(presenter.get(), &chat::SessionPresenter::sessionsLoaded);
    ASSERT_TRUE(spy.isValid());

    presenter->loadSessions();

    ASSERT_EQ(spy.count(), 1);
    auto sessions = spy.at(0).at(0).value<std::vector<chat::SessionItem>>();
    EXPECT_EQ(sessions.size(), 0u);
}

TEST_F(SessionPresenterTest, LoadSessionsWithGroup) {
    auto group = client->groups().createGroup(tokenA, {aliceId, bobId});
    ASSERT_TRUE(group.has_value());

    presenter->setSession(tokenA, aliceId);

    QSignalSpy spy(presenter.get(), &chat::SessionPresenter::sessionsLoaded);
    presenter->loadSessions();

    ASSERT_EQ(spy.count(), 1);
    auto sessions = spy.at(0).at(0).value<std::vector<chat::SessionItem>>();
    ASSERT_EQ(sessions.size(), 1u);
    EXPECT_EQ(sessions[0].chatId, group->id);
}

TEST_F(SessionPresenterTest, LoadSessionsWithMessages) {
    auto group = client->groups().createGroup(tokenA, {aliceId, bobId});
    auto chatId = group->id;

    client->chat().sendMessage(
        tokenA, chatId, 0,
        core::MessageContent{core::TextContent{"hello"}});

    presenter->setSession(tokenA, aliceId);

    QSignalSpy spy(presenter.get(), &chat::SessionPresenter::sessionsLoaded);
    presenter->loadSessions();

    auto sessions = spy.at(0).at(0).value<std::vector<chat::SessionItem>>();
    ASSERT_EQ(sessions.size(), 1u);
    EXPECT_EQ(sessions[0].lastMessage, "hello");
    EXPECT_GT(sessions[0].lastTimestamp, 0);
}

// ═══════════════════════════════════════════════════════════════
// 实时更新测试
// ═══════════════════════════════════════════════════════════════

TEST_F(SessionPresenterTest, NewMessageTriggersSessionUpdated) {
    auto group = client->groups().createGroup(tokenA, {aliceId, bobId});
    auto chatId = group->id;

    presenter->setSession(tokenA, aliceId);

    QSignalSpy spy(presenter.get(), &chat::SessionPresenter::sessionUpdated);
    ASSERT_TRUE(spy.isValid());

    // Bob 发消息，触发 messageStored → sessionUpdated
    client->chat().sendMessage(
        tokenB, chatId, 0,
        core::MessageContent{core::TextContent{"hey alice!"}});

    ASSERT_EQ(spy.count(), 1);
    auto session = spy.at(0).at(0).value<chat::SessionItem>();
    EXPECT_EQ(session.chatId, chatId);
    EXPECT_EQ(session.lastMessage, "hey alice!");
}

// ═══════════════════════════════════════════════════════════════
// 多会话排序测试
// ═══════════════════════════════════════════════════════════════

TEST_F(SessionPresenterTest, SessionsSortedByLastTimestamp) {
    auto regC = client->auth().registerUser("carol", "p");
    client->contacts().addFriend(tokenA, regC->id);

    auto group1 = client->groups().createGroup(tokenA, {aliceId, bobId});
    auto group2 = client->groups().createGroup(tokenA, {aliceId, regC->id});

    // group1 先发消息
    client->chat().sendMessage(
        tokenA, group1->id, 0,
        core::MessageContent{core::TextContent{"first"}});

    // group2 后发消息（更新）
    client->chat().sendMessage(
        tokenA, group2->id, 0,
        core::MessageContent{core::TextContent{"second"}});

    presenter->setSession(tokenA, aliceId);

    QSignalSpy spy(presenter.get(), &chat::SessionPresenter::sessionsLoaded);
    presenter->loadSessions();

    auto sessions = spy.at(0).at(0).value<std::vector<chat::SessionItem>>();
    ASSERT_EQ(sessions.size(), 2u);

    // 最新的会话在前
    EXPECT_EQ(sessions[0].chatId, group2->id);
    EXPECT_EQ(sessions[1].chatId, group1->id);
}
