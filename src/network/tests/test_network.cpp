#include <gtest/gtest.h>

#include <wechat/core/Message.h>
#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::core;
using namespace wechat::network;

class NetworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = createMockClient();
    }

    /// 注册并返回 token
    std::string registerAndLogin(const std::string& username,
                                 const std::string& password) {
        auto r = client->auth().registerUser(username, password);
        EXPECT_TRUE(r.ok());
        return r.value().token;
    }

    std::unique_ptr<NetworkClient> client;
};

// ══════════════════════════════════════════════════
// Auth
// ══════════════════════════════════════════════════

TEST_F(NetworkTest, RegisterAndLogin) {
    auto reg = client->auth().registerUser("alice", "pass123");
    ASSERT_TRUE(reg.ok());
    EXPECT_FALSE(reg.value().userId.empty());
    EXPECT_FALSE(reg.value().token.empty());

    // 用注册的凭据登录
    auto login = client->auth().login("alice", "pass123");
    ASSERT_TRUE(login.ok());
    EXPECT_EQ(login.value().userId, reg.value().userId);
}

TEST_F(NetworkTest, LoginWrongPassword) {
    registerAndLogin("bob", "secret");
    auto r = client->auth().login("bob", "wrong");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::Unauthorized);
}

TEST_F(NetworkTest, Logout) {
    auto token = registerAndLogin("carol", "pass");
    auto r = client->auth().logout(token);
    ASSERT_TRUE(r.ok());

    // token 失效
    auto user = client->auth().getCurrentUser(token);
    EXPECT_FALSE(user.ok());
}

TEST_F(NetworkTest, GetCurrentUser) {
    auto reg = client->auth().registerUser("dave", "pass");
    ASSERT_TRUE(reg.ok());

    auto user = client->auth().getCurrentUser(reg.value().token);
    ASSERT_TRUE(user.ok());
    EXPECT_EQ(user.value().id, reg.value().userId);
}

// ══════════════════════════════════════════════════
// Contacts
// ══════════════════════════════════════════════════

TEST_F(NetworkTest, AddAndListFriends) {
    auto tokenA = registerAndLogin("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto userIdB = regB.value().userId;

    auto r = client->contacts().addFriend(tokenA, userIdB);
    ASSERT_TRUE(r.ok());

    auto friends = client->contacts().listFriends(tokenA);
    ASSERT_TRUE(friends.ok());
    EXPECT_EQ(friends.value().size(), 1u);
    EXPECT_EQ(friends.value()[0].id, userIdB);
}

TEST_F(NetworkTest, RemoveFriend) {
    auto tokenA = registerAndLogin("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto userIdB = regB.value().userId;

    client->contacts().addFriend(tokenA, userIdB);
    auto r = client->contacts().removeFriend(tokenA, userIdB);
    ASSERT_TRUE(r.ok());

    auto friends = client->contacts().listFriends(tokenA);
    EXPECT_EQ(friends.value().size(), 0u);
}

TEST_F(NetworkTest, AddFriendSelf) {
    auto reg = client->auth().registerUser("alice", "p");
    auto r = client->contacts().addFriend(reg.value().token, reg.value().userId);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::InvalidArgument);
}

TEST_F(NetworkTest, SearchUser) {
    auto tokenA = registerAndLogin("alice", "p");
    registerAndLogin("bob", "p");
    registerAndLogin("bobby", "p");

    auto r = client->contacts().searchUser(tokenA, "bob");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().size(), 2u);
}

// ══════════════════════════════════════════════════
// Group
// ══════════════════════════════════════════════════

TEST_F(NetworkTest, CreateGroupAndListMembers) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenA = regA.value().token;

    auto r = client->groups().createGroup(
        tokenA, {regA.value().userId, regB.value().userId});
    ASSERT_TRUE(r.ok());
    auto groupId = r.value().id;

    auto members = client->groups().listMembers(tokenA, groupId);
    ASSERT_TRUE(members.ok());
    EXPECT_EQ(members.value().size(), 2u);
}

TEST_F(NetworkTest, DissolveGroupOnlyOwner) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    auto groupId = group.value().id;

    // bob 不是群主，不能解散
    auto r = client->groups().dissolveGroup(regB.value().token, groupId);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::PermissionDenied);

    // alice 是群主，可以解散
    auto r2 = client->groups().dissolveGroup(regA.value().token, groupId);
    ASSERT_TRUE(r2.ok());
}

TEST_F(NetworkTest, AddAndRemoveGroupMember) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto regC = client->auth().registerUser("carol", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(
        tokenA, {regA.value().userId});
    auto groupId = group.value().id;

    // 添加 bob
    auto r = client->groups().addMember(tokenA, groupId, regB.value().userId);
    ASSERT_TRUE(r.ok());

    auto members = client->groups().listMembers(tokenA, groupId);
    EXPECT_EQ(members.value().size(), 2u);

    // 移除 bob
    auto r2 = client->groups().removeMember(tokenA, groupId, regB.value().userId);
    ASSERT_TRUE(r2.ok());

    members = client->groups().listMembers(tokenA, groupId);
    EXPECT_EQ(members.value().size(), 1u);
}

TEST_F(NetworkTest, ListMyGroups) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    client->groups().createGroup(
        regA.value().token, {regA.value().userId});

    auto r = client->groups().listMyGroups(regA.value().token);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().size(), 2u);

    auto r2 = client->groups().listMyGroups(regB.value().token);
    ASSERT_TRUE(r2.ok());
    EXPECT_EQ(r2.value().size(), 1u);
}

// ══════════════════════════════════════════════════
// Chat
// ══════════════════════════════════════════════════

TEST_F(NetworkTest, SendAndSyncMessages) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenA = regA.value().token;
    auto tokenB = regB.value().token;

    auto group = client->groups().createGroup(
        tokenA, {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    // alice 发消息
    MessageContent content = {TextContent{"hello bob!"}};
    auto sent = client->chat().sendMessage(tokenA, chatId, 0, content);
    ASSERT_TRUE(sent.ok());
    EXPECT_EQ(sent.value().senderId, regA.value().userId);
    EXPECT_EQ(sent.value().chatId, chatId);

    // bob 同步
    auto sync = client->chat().fetchAfter(tokenB, chatId, 0, 50);
    ASSERT_TRUE(sync.ok());
    EXPECT_EQ(sync.value().messages.size(), 1u);
    EXPECT_FALSE(sync.value().hasMore);

    auto* text = std::get_if<TextContent>(&sync.value().messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hello bob!");
}

TEST_F(NetworkTest, SendMessageNotMember) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token, {regA.value().userId});
    auto chatId = group.value().id;

    // bob 不是成员
    MessageContent content = {TextContent{"hi"}};
    auto r = client->chat().sendMessage(regB.value().token, chatId, 0, content);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::PermissionDenied);
}

TEST_F(NetworkTest, RevokeMessage) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(tokenA, {regA.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        tokenA, chatId, 0, MessageContent{TextContent{"oops"}});
    auto msgId = sent.value().id;

    auto r = client->chat().revokeMessage(tokenA, msgId);
    ASSERT_TRUE(r.ok());

    // 同步后消息应标记为已撤回
    auto sync = client->chat().fetchAfter(tokenA, chatId, 0, 50);
    EXPECT_TRUE(sync.value().messages[0].revoked);
}

TEST_F(NetworkTest, RevokeOtherUserMessage) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token,
        {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        regA.value().token, chatId, 0, MessageContent{TextContent{"hi"}});

    // bob 不能撤回 alice 的消息
    auto r = client->chat().revokeMessage(regB.value().token, sent.value().id);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::PermissionDenied);
}

TEST_F(NetworkTest, EditMessage) {
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

TEST_F(NetworkTest, MarkRead) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token,
        {regA.value().userId, regB.value().userId});
    auto chatId = group.value().id;

    auto sent = client->chat().sendMessage(
        regA.value().token, chatId, 0, MessageContent{TextContent{"hi"}});

    auto r = client->chat().markRead(regB.value().token, chatId, sent.value().id);
    ASSERT_TRUE(r.ok());

    auto sync = client->chat().fetchAfter(regA.value().token, chatId, 0, 50);
    EXPECT_EQ(sync.value().messages[0].readCount, 1u);
}

TEST_F(NetworkTest, SyncMessagesPagination) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(tokenA, {regA.value().userId});
    auto chatId = group.value().id;

    for (int i = 0; i < 5; ++i) {
        client->chat().sendMessage(
            tokenA, chatId, 0,
            MessageContent{TextContent{"msg " + std::to_string(i)}});
    }

    // fetchAfter(0) 返回最新 3 条（msg 2,3,4），hasMore=true
    auto sync = client->chat().fetchAfter(tokenA, chatId, 0, 3);
    ASSERT_TRUE(sync.ok());
    EXPECT_EQ(sync.value().messages.size(), 3u);
    EXPECT_TRUE(sync.value().hasMore);

    // fetchBefore(0) 返回最早 3 条（msg 0,1,2），hasMore=true
    auto syncB = client->chat().fetchBefore(tokenA, chatId, 0, 3);
    ASSERT_TRUE(syncB.ok());
    EXPECT_EQ(syncB.value().messages.size(), 3u);
    EXPECT_TRUE(syncB.value().hasMore);

    // 从最早页的最后一条继续向后翻页
    auto lastId = syncB.value().messages.back().id;
    auto sync2 = client->chat().fetchAfter(tokenA, chatId, lastId, 3);
    ASSERT_TRUE(sync2.ok());
    EXPECT_EQ(sync2.value().messages.size(), 2u);
    EXPECT_FALSE(sync2.value().hasMore);
}

// ══════════════════════════════════════════════════
// Moments
// ══════════════════════════════════════════════════

TEST_F(NetworkTest, PostAndListMoments) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenA = regA.value().token;
    auto tokenB = regB.value().token;

    // 互加好友
    client->contacts().addFriend(tokenA, regB.value().userId);

    // alice 发朋友圈
    auto posted = client->moments().postMoment(tokenA, "hello world", {});
    ASSERT_TRUE(posted.ok());
    EXPECT_EQ(posted.value().authorId, regA.value().userId);

    // bob 能看到（是好友）
    auto list = client->moments().listMoments(tokenB, INT64_MAX, 50);
    ASSERT_TRUE(list.ok());
    EXPECT_EQ(list.value().size(), 1u);
    EXPECT_EQ(list.value()[0].text, "hello world");
}

TEST_F(NetworkTest, MomentsNotVisibleToStranger) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    client->moments().postMoment(regA.value().token, "secret", {});

    // bob 不是好友，看不到
    auto list = client->moments().listMoments(regB.value().token, INT64_MAX, 50);
    ASSERT_TRUE(list.ok());
    EXPECT_EQ(list.value().size(), 0u);
}

TEST_F(NetworkTest, LikeMoment) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto posted = client->moments().postMoment(tokenA, "nice day", {});
    auto momentId = posted.value().id;

    auto r = client->moments().likeMoment(tokenA, momentId);
    ASSERT_TRUE(r.ok());

    // 重复点赞
    auto r2 = client->moments().likeMoment(tokenA, momentId);
    ASSERT_FALSE(r2.ok());
    EXPECT_EQ(r2.error().code, ErrorCode::AlreadyExists);
}

TEST_F(NetworkTest, CommentMoment) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto posted = client->moments().postMoment(tokenA, "photo", {"img1"});
    auto momentId = posted.value().id;

    auto r = client->moments().commentMoment(tokenA, momentId, "great!");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().text, "great!");
    EXPECT_EQ(r.value().authorId, regA.value().userId);
}

// ══════════════════════════════════════════════════
// 端到端集成流程
// ══════════════════════════════════════════════════

TEST_F(NetworkTest, EndToEndFlow) {
    // 注册两个用户
    auto regA = client->auth().registerUser("alice", "pass");
    auto regB = client->auth().registerUser("bob", "pass");
    ASSERT_TRUE(regA.ok());
    ASSERT_TRUE(regB.ok());
    auto tokenA = regA.value().token;
    auto tokenB = regB.value().token;

    // 互加好友
    ASSERT_TRUE(client->contacts().addFriend(tokenA, regB.value().userId).ok());

    // 创建群聊
    auto group = client->groups().createGroup(
        tokenA, {regA.value().userId, regB.value().userId});
    ASSERT_TRUE(group.ok());
    auto chatId = group.value().id;

    // alice 发消息
    auto msg1 = client->chat().sendMessage(
        tokenA, chatId, 0, MessageContent{TextContent{"hey bob"}});
    ASSERT_TRUE(msg1.ok());

    // bob 同步并回复
    auto sync = client->chat().fetchAfter(tokenB, chatId, 0, 50);
    ASSERT_TRUE(sync.ok());
    EXPECT_EQ(sync.value().messages.size(), 1u);

    auto msg2 = client->chat().sendMessage(
        tokenB, chatId, msg1.value().id,
        MessageContent{TextContent{"hey alice!"}});
    ASSERT_TRUE(msg2.ok());
    EXPECT_EQ(msg2.value().replyTo, msg1.value().id);

    // alice 同步拿到 bob 的回复
    auto sync2 = client->chat().fetchAfter(
        tokenA, chatId, msg1.value().id, 50);
    ASSERT_TRUE(sync2.ok());
    EXPECT_EQ(sync2.value().messages.size(), 1u);

    auto* text = std::get_if<TextContent>(&sync2.value().messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hey alice!");

    // alice 发朋友圈
    auto moment = client->moments().postMoment(tokenA, "having fun", {});
    ASSERT_TRUE(moment.ok());

    // bob 能看到
    auto momentList = client->moments().listMoments(tokenB, INT64_MAX, 50);
    ASSERT_TRUE(momentList.ok());
    EXPECT_EQ(momentList.value().size(), 1u);
}
