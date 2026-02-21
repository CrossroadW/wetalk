#include <gtest/gtest.h>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::core;
using namespace wechat::network;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = createMockClient();
    }

    std::string registerAndLogin(const std::string& username,
                                 const std::string& password) {
        auto r = client->auth().registerUser(username, password);
        EXPECT_TRUE(r.ok());
        return r.value().token;
    }

    std::unique_ptr<NetworkClient> client;
};

TEST_F(IntegrationTest, EndToEndFlow) {
    auto regA = client->auth().registerUser("alice", "pass");
    auto regB = client->auth().registerUser("bob", "pass");
    ASSERT_TRUE(regA.ok());
    ASSERT_TRUE(regB.ok());
    auto tokenA = regA.value().token;
    auto tokenB = regB.value().token;

    ASSERT_TRUE(client->contacts().addFriend(tokenA, regB.value().userId).ok());

    auto group = client->groups().createGroup(
        tokenA, {regA.value().userId, regB.value().userId});
    ASSERT_TRUE(group.ok());
    auto chatId = group.value().id;

    auto msg1 = client->chat().sendMessage(
        tokenA, chatId, 0, MessageContent{TextContent{"hey bob"}});
    ASSERT_TRUE(msg1.ok());

    auto sync = client->chat().fetchAfter(tokenB, chatId, 0, 50);
    ASSERT_TRUE(sync.ok());
    EXPECT_EQ(sync.value().messages.size(), 1u);

    auto msg2 = client->chat().sendMessage(
        tokenB, chatId, msg1.value().id,
        MessageContent{TextContent{"hey alice!"}});
    ASSERT_TRUE(msg2.ok());
    EXPECT_EQ(msg2.value().replyTo, msg1.value().id);

    auto sync2 = client->chat().fetchAfter(
        tokenA, chatId, msg1.value().id, 50);
    ASSERT_TRUE(sync2.ok());
    EXPECT_EQ(sync2.value().messages.size(), 1u);

    auto* text = std::get_if<TextContent>(&sync2.value().messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hey alice!");

    auto moment = client->moments().postMoment(tokenA, "having fun", {});
    ASSERT_TRUE(moment.ok());

    auto momentList = client->moments().listMoments(tokenB, INT64_MAX, 50);
    ASSERT_TRUE(momentList.ok());
    EXPECT_EQ(momentList.value().size(), 1u);
}

TEST_F(IntegrationTest, MultiUserGroupChat) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto regC = client->auth().registerUser("carol", "p");

    auto group = client->groups().createGroup(
        regA.value().token,
        {regA.value().userId, regB.value().userId, regC.value().userId});
    auto chatId = group.value().id;

    // alice 发消息
    client->chat().sendMessage(
        regA.value().token, chatId, 0, MessageContent{TextContent{"hello"}});
    // bob 回复
    client->chat().sendMessage(
        regB.value().token, chatId, 0, MessageContent{TextContent{"hi"}});

    // carol 同步两条消息
    auto sync = client->chat().fetchAfter(regC.value().token, chatId, 0, 50);
    EXPECT_EQ(sync.value().messages.size(), 2u);
}

TEST_F(IntegrationTest, CompleteFriendshipFlow) {
    auto tokenA = registerAndLogin("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenB = regB.value().token;
    auto regC = client->auth().registerUser("carol", "p");

    // alice 搜索并添加 bob
    auto search = client->contacts().searchUser(tokenA, "bob");
    ASSERT_TRUE(search.ok());
    ASSERT_EQ(search.value().size(), 1u);

    ASSERT_TRUE(client->contacts().addFriend(tokenA, regB.value().userId).ok());

    // alice 发朋友圈
    client->moments().postMoment(tokenA, "hello friends", {});

    // bob 能看到
    auto moments = client->moments().listMoments(tokenB, INT64_MAX, 50);
    EXPECT_EQ(moments.value().size(), 1u);

    // carol 看不到
    auto momentsC = client->moments().listMoments(regC.value().token, INT64_MAX, 50);
    EXPECT_EQ(momentsC.value().size(), 0u);
}
