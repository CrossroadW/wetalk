#include <gtest/gtest.h>

#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::network;

class ContactTest : public ::testing::Test {
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

TEST_F(ContactTest, AddAndListFriends) {
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

TEST_F(ContactTest, RemoveFriend) {
    auto tokenA = registerAndLogin("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto userIdB = regB.value().userId;

    client->contacts().addFriend(tokenA, userIdB);
    auto r = client->contacts().removeFriend(tokenA, userIdB);
    ASSERT_TRUE(r.ok());

    auto friends = client->contacts().listFriends(tokenA);
    EXPECT_EQ(friends.value().size(), 0u);
}

TEST_F(ContactTest, AddFriendSelf) {
    auto reg = client->auth().registerUser("alice", "p");
    auto r = client->contacts().addFriend(reg.value().token, reg.value().userId);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::InvalidArgument);
}

TEST_F(ContactTest, SearchUser) {
    auto tokenA = registerAndLogin("alice", "p");
    registerAndLogin("bob", "p");
    registerAndLogin("bobby", "p");

    auto r = client->contacts().searchUser(tokenA, "bob");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().size(), 2u);
}

TEST_F(ContactTest, SearchUserEmptyResult) {
    auto tokenA = registerAndLogin("alice", "p");
    auto r = client->contacts().searchUser(tokenA, "nonexistent");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().size(), 0u);
}

TEST_F(ContactTest, BidirectionalFriendship) {
    auto tokenA = registerAndLogin("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenB = regB.value().token;

    // alice 添加 bob
    client->contacts().addFriend(tokenA, regB.value().userId);

    // 互为好友
    auto friendsA = client->contacts().listFriends(tokenA);
    auto friendsB = client->contacts().listFriends(tokenB);
    EXPECT_EQ(friendsA.value().size(), 1u);
    EXPECT_EQ(friendsB.value().size(), 1u);
}
