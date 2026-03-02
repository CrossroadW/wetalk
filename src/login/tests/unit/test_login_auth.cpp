#include "LoginTestFixture.h"

// ═══════════════════════════════════════════════════════════════
// 登录测试
// ═══════════════════════════════════════════════════════════════

TEST_F(LoginPresenterTest, LoginSuccess) {
    // 先注册
    client->auth().registerUser("carol", "secret");

    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("carol", "secret");

    ASSERT_TRUE(waitForSignal(successSpy, 3000)) << "Timeout waiting for loginSuccess signal";
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

    ASSERT_TRUE(waitForSignal(failSpy, 3000)) << "Timeout waiting for loginFailed signal";
    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(LoginPresenterTest, LoginUnknownUser) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->login("nobody", "pass");

    ASSERT_TRUE(waitForSignal(failSpy, 3000)) << "Timeout waiting for loginFailed signal";
    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}
