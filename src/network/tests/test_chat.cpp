#include <gtest/gtest.h>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::core;
using namespace wechat::network;

class ChatTest : public ::testing::Test {
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

TEST_F(ChatTest, SendAndSyncMessages) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenA = regA.value().token;
    auto tokenB = regB.value().token;

    auto group = client->groups().createGroup(
        tokenA, {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    MessageContent content = {TextContent{"hello bob!"}};
    auto sent = client->chat().sendMessage(tokenA, chatId, 0, content);
    ASSERT_TRUE(sent.ok());
    EXPECT_EQ(sent.value().senderId, regA.value().userId);

    auto sync = client->chat().fetchAfter(tokenB, chatId, 0, 50);
    ASSERT_TRUE(sync.ok());
    EXPECT_EQ(sync.value().messages.size(), 1u);

    auto* text = std::get_if<TextContent>(&sync.value().messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hello bob!");
}

TEST_F(ChatTest, SendMessageNotMember) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(regA.value().token, {regA.value().userId});
    auto chatId = group.value().id;

    MessageContent content = {TextContent{"hi"}};
    auto r = client->chat().sendMessage(regB.value().token, chatId, 0, content);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::PermissionDenied);
}

TEST_F(ChatTest, RevokeMessage) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(tokenA, {regA.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        tokenA, chatId, 0, MessageContent{TextContent{"oops"}});
    auto msgId = sent.value().id;

    auto r = client->chat().revokeMessage(tokenA, msgId);
    ASSERT_TRUE(r.ok());

    auto sync = client->chat().fetchAfter(tokenA, chatId, 0, 50);
    EXPECT_TRUE(sync.value().messages[0].revoked);
}

TEST_F(ChatTest, RevokeOtherUserMessage) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        regA.value().token, chatId, 0, MessageContent{TextContent{"hi"}});

    auto r = client->chat().revokeMessage(regB.value().token, sent.value().id);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::PermissionDenied);
}

TEST_F(ChatTest, EditMessage) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(tokenA, {regA.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        tokenA, chatId, 0, MessageContent{TextContent{"typo"}});
    auto msgId = sent.value().id;

    MessageContent newContent = {TextContent{"fixed"}};
    auto r = client->chat().editMessage(tokenA, msgId, newContent);
    ASSERT_TRUE(r.ok());

    auto sync = client->chat().fetchAfter(tokenA, chatId, 0, 50);
    auto* text = std::get_if<TextContent>(&sync.value().messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "fixed");
    EXPECT_GT(sync.value().messages[0].editedAt, 0);
}

TEST_F(ChatTest, MarkRead) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        regA.value().token, chatId, 0, MessageContent{TextContent{"hi"}});

    auto r = client->chat().markRead(regB.value().token, chatId, sent.value().id);
    ASSERT_TRUE(r.ok());

    auto sync = client->chat().fetchAfter(regA.value().token, chatId, 0, 50);
    EXPECT_EQ(sync.value().messages[0].readCount, 1u);
}

TEST_F(ChatTest, SyncMessagesPagination) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(tokenA, {regA.value().userId});
    auto chatId = group.value().id;

    for (int i = 0; i < 5; ++i) {
        client->chat().sendMessage(
            tokenA, chatId, 0,
            MessageContent{TextContent{"msg " + std::to_string(i)}});
    }

    auto sync = client->chat().fetchAfter(tokenA, chatId, 0, 3);
    ASSERT_TRUE(sync.ok());
    EXPECT_EQ(sync.value().messages.size(), 3u);
    EXPECT_TRUE(sync.value().hasMore);

    auto lastId = sync.value().messages.back().id;
    auto sync2 = client->chat().fetchAfter(tokenA, chatId, lastId, 3);
    ASSERT_TRUE(sync2.ok());
    EXPECT_EQ(sync2.value().messages.size(), 2u);
    EXPECT_FALSE(sync2.value().hasMore);
}

TEST_F(ChatTest, SendMessageReplyTo) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    auto msg1 = client->chat().sendMessage(
        regA.value().token, chatId, 0,
        MessageContent{TextContent{"original"}});
    auto msg2 = client->chat().sendMessage(
        regB.value().token, chatId, msg1.value().id,
        MessageContent{TextContent{"reply"}});

    ASSERT_TRUE(msg2.ok());
    EXPECT_EQ(msg2.value().replyTo, msg1.value().id);
}

TEST_F(ChatTest, SendEmptyMessage) {
    auto token = registerAndLogin("alice", "p");

    auto group = client->groups().createGroup(token, {"u1"});
    auto chatId = group.value().id;

    MessageContent content;
    auto r = client->chat().sendMessage(token, chatId, 0, content);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::InvalidArgument);
}
