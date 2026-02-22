#include <gtest/gtest.h>

#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::network;

class AuthTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = createMockClient();
    }

    std::unique_ptr<NetworkClient> client;
};

// ══════════════════════════════════════════════════
// Auth Tests
// ══════════════════════════════════════════════════

TEST_F(AuthTest, RegisterAndLogin) {
    auto reg = client->auth().registerUser("alice", "pass123");
    ASSERT_TRUE(reg.has_value());
    EXPECT_NE(reg->id, 0);
    EXPECT_EQ(reg->username, "alice");
    EXPECT_EQ(reg->password, "pass123");
    EXPECT_FALSE(reg->token.empty());

    auto login = client->auth().login("alice", "pass123");
    ASSERT_TRUE(login.has_value());
    EXPECT_EQ(login->id, reg->id);
    EXPECT_FALSE(login->token.empty());
}

TEST_F(AuthTest, LoginWrongPassword) {
    client->auth().registerUser("bob", "secret");
    auto r = client->auth().login("bob", "wrong");
    ASSERT_FALSE(r.has_value());
}

TEST_F(AuthTest, Logout) {
    auto reg = client->auth().registerUser("carol", "pass");
    auto r = client->auth().logout(reg->token);
    ASSERT_TRUE(r.has_value());

    auto user = client->auth().getCurrentUser(reg->token);
    EXPECT_FALSE(user.has_value());
}

TEST_F(AuthTest, GetCurrentUser) {
    auto reg = client->auth().registerUser("dave", "pass");
    ASSERT_TRUE(reg.has_value());

    auto user = client->auth().getCurrentUser(reg->token);
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->id, reg->id);
    EXPECT_EQ(user->username, "dave");
    EXPECT_EQ(user->token, reg->token);
}

TEST_F(AuthTest, RegisterDuplicateUsername) {
    auto r1 = client->auth().registerUser("alice", "pass");
    auto r2 = client->auth().registerUser("alice", "other");
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_NE(r1->id, r2->id);
}

TEST_F(AuthTest, LoginUnknownUser) {
    auto r = client->auth().login("nobody", "pass");
    ASSERT_FALSE(r.has_value());
}

TEST_F(AuthTest, InvalidToken) {
    auto user = client->auth().getCurrentUser("invalid_token");
    ASSERT_FALSE(user.has_value());
}

TEST_F(AuthTest, EmptyUsernameOrPassword) {
    auto r1 = client->auth().registerUser("", "pass");
    ASSERT_FALSE(r1.has_value());

    auto r2 = client->auth().registerUser("user", "");
    ASSERT_FALSE(r2.has_value());
}

TEST_F(AuthTest, LogoutInvalidToken) {
    auto r = client->auth().logout("nonexistent_token");
    ASSERT_FALSE(r.has_value());
}

TEST_F(AuthTest, LoginRefreshesToken) {
    auto reg = client->auth().registerUser("eve", "pass");
    auto firstToken = reg->token;

    auto login = client->auth().login("eve", "pass");
    ASSERT_TRUE(login.has_value());
    EXPECT_NE(login->token, firstToken);
}
