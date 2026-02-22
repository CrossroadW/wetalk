#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include <wechat/core/User.h>
#include <wechat/login/LoginPresenter.h>
#include <wechat/network/NetworkClient.h>

using namespace wechat;

Q_DECLARE_METATYPE(core::User)

class LoginPresenterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_login";
            static char* argv[] = {arg0};
            static QCoreApplication app(argc, argv);
        }
        qRegisterMetaType<core::User>();
    }

    void SetUp() override {
        client = network::createMockClient();
        presenter = std::make_unique<login::LoginPresenter>(*client);
    }

    std::unique_ptr<network::NetworkClient> client;
    std::unique_ptr<login::LoginPresenter> presenter;
};

// ═══════════════════════════════════════════════════════════════
// 注册测试
// ═══════════════════════════════════════════════════════════════

TEST_F(LoginPresenterTest, RegisterSuccess) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);
    ASSERT_TRUE(successSpy.isValid());
    ASSERT_TRUE(failSpy.isValid());

    presenter->registerUser("alice", "pass123");

    ASSERT_EQ(successSpy.count(), 1);
    EXPECT_EQ(failSpy.count(), 0);

    auto user = successSpy.at(0).at(0).value<core::User>();
    EXPECT_NE(user.id, 0);
    EXPECT_EQ(user.username, "alice");
    EXPECT_FALSE(user.token.empty());
}

TEST_F(LoginPresenterTest, RegisterEmptyUsername) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->registerUser("", "pass");

    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(LoginPresenterTest, RegisterEmptyPassword) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->registerUser("bob", "");

    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}

// ═══════════════════════════════════════════════════════════════
// 登录测试
// ═══════════════════════════════════════════════════════════════

TEST_F(LoginPresenterTest, LoginSuccess) {
    // 先注册
    client->auth().registerUser("carol", "secret");

    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("carol", "secret");

    ASSERT_EQ(successSpy.count(), 1);
    EXPECT_EQ(failSpy.count(), 0);

    auto user = successSpy.at(0).at(0).value<core::User>();
    EXPECT_EQ(user.username, "carol");
    EXPECT_FALSE(user.token.empty());
}

TEST_F(LoginPresenterTest, LoginWrongPassword) {
    client->auth().registerUser("dave", "correct");

    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("dave", "wrong");

    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(LoginPresenterTest, LoginUnknownUser) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("nobody", "pass");

    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}

// ═══════════════════════════════════════════════════════════════
// 信号内容验证
// ═══════════════════════════════════════════════════════════════

TEST_F(LoginPresenterTest, RegisterReturnsValidUser) {
    QSignalSpy spy(presenter.get(), &login::LoginPresenter::loginSuccess);

    presenter->registerUser("eve", "password");

    auto user = spy.at(0).at(0).value<core::User>();
    EXPECT_NE(user.id, 0);
    EXPECT_EQ(user.username, "eve");
    EXPECT_EQ(user.password, "password");
    EXPECT_FALSE(user.token.empty());
}

TEST_F(LoginPresenterTest, LoginRefreshesToken) {
    auto reg = client->auth().registerUser("frank", "pass");
    auto firstToken = reg->token;

    QSignalSpy spy(presenter.get(), &login::LoginPresenter::loginSuccess);

    presenter->login("frank", "pass");

    auto user = spy.at(0).at(0).value<core::User>();
    EXPECT_NE(user.token, firstToken);
}

TEST_F(LoginPresenterTest, FailedSignalContainsMessage) {
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("nonexistent", "pass");

    ASSERT_EQ(failSpy.count(), 1);
    auto message = failSpy.at(0).at(0).value<QString>();
    EXPECT_FALSE(message.isEmpty());
}
