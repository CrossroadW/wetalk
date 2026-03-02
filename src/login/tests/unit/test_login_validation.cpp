#include "LoginTestFixture.h"

// ═══════════════════════════════════════════════════════════════
// 信号内容验证
// ═══════════════════════════════════════════════════════════════

TEST_F(LoginPresenterTest, RegisterReturnsValidUser) {
    QSignalSpy spy(presenter.get(), &login::LoginPresenter::loginSuccess);

    presenter->registerUser("eve", "password");

    ASSERT_TRUE(waitForSignal(spy, 3000)) << "Timeout waiting for loginSuccess signal";
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

    ASSERT_TRUE(waitForSignal(spy, 3000)) << "Timeout waiting for loginSuccess signal";
    auto user = spy.at(0).at(0).value<core::User>();
    EXPECT_NE(user.token, firstToken);
}

TEST_F(LoginPresenterTest, FailedSignalContainsMessage) {
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("nonexistent", "pass");

    ASSERT_TRUE(waitForSignal(failSpy, 3000)) << "Timeout waiting for loginFailed signal";
    ASSERT_EQ(failSpy.count(), 1);
    auto message = failSpy.at(0).at(0).value<QString>();
    EXPECT_FALSE(message.isEmpty());
}
