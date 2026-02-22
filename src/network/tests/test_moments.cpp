#include <gtest/gtest.h>

#include <wechat/network/NetworkClient.h>
#include <wechat/network/NetworkTypes.h>

using namespace wechat::network;

class MomentTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = createMockClient();
    }

    std::string registerAndLogin(const std::string& username,
                                 const std::string& password) {
        auto r = client->auth().registerUser(username, password);
        EXPECT_TRUE(r.has_value());
        return r.value().token;
    }

    std::unique_ptr<NetworkClient> client;
};

TEST_F(MomentTest, PostAndListMoments) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenA = regA.value().token;
    auto tokenB = regB.value().token;

    client->contacts().addFriend(tokenA, regB.value().userId);

    auto posted = client->moments().postMoment(tokenA, "hello world", {});
    ASSERT_TRUE(posted.has_value());
    EXPECT_EQ(posted.value().authorId, regA.value().userId);

    auto list = client->moments().listMoments(tokenB, INT64_MAX, 50);
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ(list.value().size(), 1u);
    EXPECT_EQ(list.value()[0].text, "hello world");
}

TEST_F(MomentTest, MomentsNotVisibleToStranger) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");

    client->moments().postMoment(regA.value().token, "secret", {});

    auto list = client->moments().listMoments(regB.value().token, INT64_MAX, 50);
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ(list.value().size(), 0u);
}

TEST_F(MomentTest, LikeMoment) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto posted = client->moments().postMoment(tokenA, "nice day", {});
    auto momentId = posted.value().id;

    auto r = client->moments().likeMoment(tokenA, momentId);
    ASSERT_TRUE(r.has_value());

    auto r2 = client->moments().likeMoment(tokenA, momentId);
    ASSERT_FALSE(r2.has_value());
}

TEST_F(MomentTest, CommentMoment) {
    auto regA = client->auth().registerUser("alice", "p");
    auto tokenA = regA.value().token;

    auto posted = client->moments().postMoment(tokenA, "photo", {"img1"});
    auto momentId = posted.value().id;

    auto r = client->moments().commentMoment(tokenA, momentId, "great!");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().text, "great!");
    EXPECT_EQ(r.value().authorId, regA.value().userId);
}

TEST_F(MomentTest, PostMomentWithImages) {
    auto token = registerAndLogin("alice", "p");

    auto posted = client->moments().postMoment(
        token, "", {"img1.jpg", "img2.jpg"});
    ASSERT_TRUE(posted.has_value());
    EXPECT_EQ(posted.value().imageIds.size(), 2u);
}

TEST_F(MomentTest, PostEmptyMoment) {
    auto token = registerAndLogin("alice", "p");

    auto r = client->moments().postMoment(token, "", {});
    ASSERT_FALSE(r.has_value());
}

TEST_F(MomentTest, ListMomentsPagination) {
    auto regA = client->auth().registerUser("alice", "p");
    auto regB = client->auth().registerUser("bob", "p");
    auto tokenB = regB.value().token;

    client->contacts().addFriend(regA.value().token, regB.value().userId);

    for (int i = 0; i < 5; ++i) {
        client->moments().postMoment(
            regA.value().token, "post " + std::to_string(i), {});
    }

    auto list = client->moments().listMoments(tokenB, INT64_MAX, 3);
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ(list.value().size(), 3u);
}

TEST_F(MomentTest, LikeNonExistentMoment) {
    auto token = registerAndLogin("alice", "p");

    auto r = client->moments().likeMoment(token, 999999);
    ASSERT_FALSE(r.has_value());
}
