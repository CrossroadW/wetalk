#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

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
        // 使用 WebSocket 客户端连接真实后端
        // 如果连接失败会抛出异常，测试会失败（不是 SKIP）
        try {
            client = network::createWsClient("ws://localhost:8000/ws");
            presenter = std::make_unique<login::LoginPresenter>(*client);
        } catch (const std::exception& e) {
            FAIL() << "Failed to connect to backend: " << e.what()
                   << "\nMake sure backend is running at ws://localhost:8000/ws";
        }
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

// ═══════════════════════════════════════════════════════════════
// 二维码登录测试
// ═══════════════════════════════════════════════════════════════

// 场景 01: 首次启动显示二维码
TEST_F(LoginPresenterTest, QRLogin_01_Initial) {
    QSignalSpy qrReadySpy(presenter.get(), &login::LoginPresenter::qrCodeReady);
    ASSERT_TRUE(qrReadySpy.isValid());

    presenter->startQRLogin();

    // 验证二维码生成信号
    ASSERT_EQ(qrReadySpy.count(), 1);
    auto qrUrl = qrReadySpy.at(0).at(0).value<QString>();
    auto sessionId = qrReadySpy.at(0).at(1).value<QString>();

    EXPECT_FALSE(qrUrl.isEmpty());
    EXPECT_FALSE(sessionId.isEmpty());
    EXPECT_TRUE(qrUrl.contains("qr-login"));
    EXPECT_TRUE(qrUrl.contains(sessionId));
}

// 场景 03: 扫码后但未在手机上确认
TEST_F(LoginPresenterTest, QRLogin_03_Scanned) {
    QSignalSpy qrReadySpy(presenter.get(), &login::LoginPresenter::qrCodeReady);
    QSignalSpy qrScannedSpy(presenter.get(), &login::LoginPresenter::qrCodeScanned);
    QSignalSpy loginSuccessSpy(presenter.get(), &login::LoginPresenter::loginSuccess);

    // 启动二维码登录
    presenter->startQRLogin();
    ASSERT_EQ(qrReadySpy.count(), 1);

    // TODO: 模拟扫码但不确认
    // 需要后端支持 qr_scanned 推送消息
    // 此时应该收到 qrCodeScanned 信号，但不应该收到 loginSuccess 信号

    // 验证状态：已扫码但未登录
    // EXPECT_EQ(qrScannedSpy.count(), 1);
    // EXPECT_EQ(loginSuccessSpy.count(), 0);
}

// 场景 02: 手机确认后的加载状态
TEST_F(LoginPresenterTest, QRLogin_02_Loading) {
    QSignalSpy qrReadySpy(presenter.get(), &login::LoginPresenter::qrCodeReady);
    QSignalSpy loginSuccessSpy(presenter.get(), &login::LoginPresenter::loginSuccess);

    // 启动二维码登录
    presenter->startQRLogin();
    ASSERT_EQ(qrReadySpy.count(), 1);
    auto sessionId = qrReadySpy.at(0).at(1).value<QString>();

    // TODO: 调用后端 /api/qr-confirm 确认登录
    // 需要使用 HTTP 客户端调用: POST /api/qr-confirm {"session_id": sessionId, "username": "alice"}
    // 后端会推送 qr_confirmed 消息

    // 等待推送和登录成功
    QTest::qWait(100);

    // 验证登录成功
    // ASSERT_EQ(loginSuccessSpy.count(), 1);
    // auto user = loginSuccessSpy.at(0).at(0).value<core::User>();
    // EXPECT_NE(user.id, 0);
    // EXPECT_FALSE(user.username.empty());
    // EXPECT_FALSE(user.token.empty());
}

// 场景 04: 有有效 token 直接登录（跳过二维码）
TEST_F(LoginPresenterTest, QRLogin_04_DirectWithToken) {
    // 先注册获取 token
    auto reg = client->auth().registerUser("grace", "pass123");
    ASSERT_TRUE(reg.has_value());
    auto token = reg->token;

    QSignalSpy loginSuccessSpy(presenter.get(), &login::LoginPresenter::loginSuccess);
    QSignalSpy qrReadySpy(presenter.get(), &login::LoginPresenter::qrCodeReady);

    // TODO: 使用 token 直接登录
    // presenter->loginWithToken(token);

    // 验证：直接登录成功，不应该生成二维码
    // ASSERT_EQ(loginSuccessSpy.count(), 1);
    // EXPECT_EQ(qrReadySpy.count(), 0);

    // auto user = loginSuccessSpy.at(0).at(0).value<core::User>();
    // EXPECT_EQ(user.username, "grace");
    // EXPECT_EQ(user.token, token);
}

// 边界测试: 连续请求多次二维码
TEST_F(LoginPresenterTest, QRLogin_MultipleRequests) {
    QSignalSpy qrReadySpy(presenter.get(), &login::LoginPresenter::qrCodeReady);

    // 连续请求多次二维码
    presenter->startQRLogin();
    presenter->startQRLogin();
    presenter->startQRLogin();

    // 每次请求都应该生成新的二维码
    ASSERT_EQ(qrReadySpy.count(), 3);

    auto sessionId1 = qrReadySpy.at(0).at(1).value<QString>();
    auto sessionId2 = qrReadySpy.at(1).at(1).value<QString>();
    auto sessionId3 = qrReadySpy.at(2).at(1).value<QString>();

    // 每次的 session_id 应该不同
    EXPECT_NE(sessionId1, sessionId2);
    EXPECT_NE(sessionId2, sessionId3);
    EXPECT_NE(sessionId1, sessionId3);
}
