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
    EXPECT_NE(reg.value().userId, 0);
    EXPECT_FALSE(reg.value().token.empty());

    auto login = client->auth().login("alice", "pass123");
    ASSERT_TRUE(login.has_value());
    EXPECT_EQ(login.value().userId, reg.value().userId);
}

TEST_F(AuthTest, LoginWrongPassword) {
    client->auth().registerUser("bob", "secret");
    auto r = client->auth().login("bob", "wrong");
    ASSERT_FALSE(r.has_value());
}

TEST_F(AuthTest, Logout) {
    auto reg = client->auth().registerUser("carol", "pass");
    auto r = client->auth().logout(reg.value().token);
    ASSERT_TRUE(r.has_value());

    auto user = client->auth().getCurrentUser(reg.value().token);
    EXPECT_FALSE(user.has_value());
}

TEST_F(AuthTest, GetCurrentUser) {
    auto reg = client->auth().registerUser("dave", "pass");
    ASSERT_TRUE(reg.has_value());

    auto user = client->auth().getCurrentUser(reg.value().token);
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user.value().id, reg.value().userId);
}

TEST_F(AuthTest, RegisterDuplicateUsername) {
    client->auth().registerUser("alice", "pass");
    auto r = client->auth().registerUser("alice", "other");
    ASSERT_FALSE(r.has_value());
}

TEST_F(AuthTest, LoginUnknownUser) {
    auto r = client->auth().login("nobody", "pass");
    ASSERT_FALSE(r.has_value());
}

TEST_F(AuthTest, InvalidToken) {
    auto user = client->auth().getCurrentUser("invalid_token");
    ASSERT_FALSE(user.has_value());
}
