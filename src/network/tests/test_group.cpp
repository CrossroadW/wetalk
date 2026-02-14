#include <gtest/gtest.h>

#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::network;

class GroupTest : public ::testing::Test {
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

TEST_F(GroupTest, CreateGroupAndListMembers) {
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

TEST_F(GroupTest, DissolveGroupOnlyOwner) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    auto group = client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    auto groupId = group.value().id;

    auto r = client->groups().dissolveGroup(regB.value().token, groupId);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::PermissionDenied);

    auto r2 = client->groups().dissolveGroup(regA.value().token, groupId);
    ASSERT_TRUE(r2.ok());
}

TEST_F(GroupTest, AddAndRemoveGroupMember) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto regC = client->auth().registerUser("carol", "p");
    auto tokenA = regA.value().token;

    auto group = client->groups().createGroup(tokenA, {regA.value().userId});
    auto groupId = group.value().id;

    auto r = client->groups().addMember(tokenA, groupId, regB.value().userId);
    ASSERT_TRUE(r.ok());

    auto members = client->groups().listMembers(tokenA, groupId);
    EXPECT_EQ(members.value().size(), 2u);

    auto r2 = client->groups().removeMember(tokenA, groupId, regB.value().userId);
    ASSERT_TRUE(r2.ok());

    members = client->groups().listMembers(tokenA, groupId);
    EXPECT_EQ(members.value().size(), 1u);
}

TEST_F(GroupTest, ListMyGroups) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    client->groups().createGroup(
        regA.value().token, {regA.value().userId, regB.value().userId});
    client->groups().createGroup(regA.value().token, {regA.value().userId});

    auto r = client->groups().listMyGroups(regA.value().token);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().size(), 2u);

    auto r2 = client->groups().listMyGroups(regB.value().token);
    ASSERT_TRUE(r2.ok());
    EXPECT_EQ(r2.value().size(), 1u);
}

TEST_F(GroupTest, CreateGroupEmptyMembers) {
    auto reg = client->auth().registerUser("alice", "p");
    auto r = client->groups().createGroup(reg.value().token, {});
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().ownerId, reg.value().userId);
    EXPECT_EQ(r.value().memberIds.size(), 1u);
}

TEST_F(GroupTest, DissolveNonExistentGroup) {
    auto token = registerAndLogin("alice", "p");
    auto r = client->groups().dissolveGroup(token, "nonexistent");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code, ErrorCode::NotFound);
}
