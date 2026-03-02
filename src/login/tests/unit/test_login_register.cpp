#include "LoginTestFixture.h"

// ═══════════════════════════════════════════════════════════════
// 注册测试
// ═══════════════════════════════════════════════════════════════

TEST_F(LoginPresenterTest, RegisterSuccess) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);
    ASSERT_TRUE(successSpy.isValid());
    ASSERT_TRUE(failSpy.isValid());

    presenter->registerUser("alice", "pass123");

    ASSERT_TRUE(waitForSignal(successSpy, 3000)) << "Timeout waiting for loginSuccess signal";
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

    ASSERT_TRUE(waitForSignal(failSpy, 1000)) << "Timeout waiting for loginFailed signal";
    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(LoginPresenterTest, RegisterEmptyPassword) {
    QSignalSpy successSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy failSpy(presenter.get(), &login::LoginPresenter::loginFailed);

    presenter->registerUser("bob", "");

    ASSERT_TRUE(waitForSignal(failSpy, 1000)) << "Timeout waiting for loginFailed signal";
    EXPECT_EQ(successSpy.count(), 0);
    ASSERT_EQ(failSpy.count(), 1);
}
