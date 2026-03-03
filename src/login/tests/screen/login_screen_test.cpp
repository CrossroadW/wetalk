#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <network/WsClient.h>
#include "LoginWidget.h"

// 截图测试 — 使用真实 WebSocket 连接后端
// 要求：后端运行在 ws://127.0.0.1:8000/ws
class LoginScreenTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
        QDir rootDir(PROJECT_ROOT_PATH);
        QString outputPath = rootDir.filePath("tests/screenshots/login/output");
        QDir().mkpath(outputPath);
        QDir::setCurrent(outputPath);
        qDebug() << "Output directory:" << QDir::currentPath();

        m_wsClient = new wechat::network::WsClient(this);

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(3000);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(m_wsClient, &wechat::network::WebSocketClient::connected, [&]() {
            qDebug() << "✅ WebSocket connected";
            loop.quit();
        });
        connect(m_wsClient, &wechat::network::WebSocketClient::error, [&](const QString& err) {
            qWarning() << "❌ WebSocket error:" << err;
            loop.quit();
        });

        qDebug() << "Connecting to ws://127.0.0.1:8000/ws...";
        m_wsClient->connectToServer("ws://127.0.0.1:8000/ws");
        timeout.start();
        loop.exec();

        m_connected = m_wsClient->isConnected();
        if (!m_connected)
            qWarning() << "⚠️  Backend not available at ws://127.0.0.1:8000/ws";
        else
            qDebug() << "✅ Backend connected successfully";
    }

    void cleanupTestCase() {
        if (!m_connected) return;
        QNetworkAccessManager nam;
        QNetworkRequest req(QUrl("http://127.0.0.1:8000/api/test/reset-db"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(2000);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        auto* reply = nam.post(req, QByteArray{});
        connect(reply, &QNetworkReply::finished, [&]() {
            reply->deleteLater();
            loop.quit();
        });

        timer.start();
        loop.exec();
        qDebug() << "✅ Database reset";
    }

    void init() {
        m_loginWidget = new wechat::login::LoginWidget(m_wsClient);
        m_loginWidget->setObjectName("LoginWidget");
    }

    void cleanup() {
        delete m_loginWidget;
        m_loginWidget = nullptr;
    }

    // ── Test: QR 码生成后的等待扫码状态 ────────────────────────────
    void test_qr_code() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test: QR Code ===";

        bool gotResponse = false;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(1500);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

        auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                [&](const QString& type, const QJsonObject& data) {
            if (type == "qr_login_init") {
                qDebug() << "✅ session_id:" << data["session_id"].toString();
                gotResponse = true;
                loop.quit();
            }
        });

        QJsonObject req;
        req["type"] = "qr_login_init";
        req["data"] = QJsonObject{};
        m_wsClient->send(req);

        timeout.start();
        loop.exec();
        disconnect(conn);

        QVERIFY2(gotResponse, "Did not receive qr_login_init response");
        // 通过 friend access 验证 widget 内部状态
        QVERIFY(!m_loginWidget->currentSessionId.isEmpty());
        saveScreenshot("qr_code.png");
    }

    // ── Test: 初始加载状态（连接建立但还未请求 QR 码）─────────────
    void test_qr_entering() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test: QR Entering (initial loading state) ===";

        // Widget 创建后立刻就是 Loading 状态，无需发送任何请求
        QCOMPARE(m_loginWidget->qrStatusLabel->text(), QString("Connecting to server..."));
        saveScreenshot("qr_entering.png");
    }

    // ── Test: 连接失败状态 ─────────────────────────────────────────
    void test_connection_failed() {
        // 此测试不需要后端连接
        qDebug() << "\n=== Test: Connection Failed ===";

        // 创建一个没有 WebSocket 客户端的 widget
        auto* widget = new wechat::login::LoginWidget(nullptr);
        widget->setObjectName("LoginWidget");
        widget->showConnectionFailed();

        // 验证状态
        QCOMPARE(widget->qrStatusLabel->text(), QString("Cannot connect to server. Retrying..."));

        // 保存截图
        widget->show();
        (void)QTest::qWaitForWindowExposed(widget);
        QTest::qWait(100);
        QPixmap pix = widget->grab();
        pix.toImage().save("connection_failed.png", "PNG", 70);
        qDebug() << "📸 Screenshot saved: connection_failed.png Size:" << pix.size();

        delete widget;
    }

    // ── Test: 模拟扫码 — POST /api/qr-confirm 触发 qr_scanned 推送 ─
    void test_qr_scanned() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test: QR Scanned ===";

        // 注册测试用户（qr-confirm 要求用户已存在）
        const QString testUser = "qr_scan_test";
        const QString testPass = "pass123";
        if (!getUserToken(testUser, testPass).isEmpty())
            qDebug() << "✅ Test user ready";
        else
            QSKIP("Failed to prepare test user");

        // 发送 qr_login_init，widget 自动处理响应更新 currentSessionId
        QString sessionId;
        {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.setInterval(1500);
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

            auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                    [&](const QString& type, const QJsonObject& data) {
                if (type == "qr_login_init") {
                    sessionId = data["session_id"].toString();
                    loop.quit();
                }
            });

            QJsonObject req;
            req["type"] = "qr_login_init";
            req["data"] = QJsonObject{};
            m_wsClient->send(req);

            timeout.start();
            loop.exec();
            disconnect(conn);
        }

        if (sessionId.isEmpty()) QSKIP("Failed to get session_id");
        // 通过 friend access 验证 widget 已同步到同一 session_id
        QCOMPARE(m_loginWidget->currentSessionId, sessionId);

        qDebug() << "📤 Sending POST /api/qr-confirm with session_id:" << sessionId;

        // POST /api/qr-confirm — backend 会先推 qr_scanned，再推 qr_confirmed
        // 等待 HTTP 响应确认 backend 已完成推送，再检查 widget 状态
        bool httpSuccess = false;
        {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.setInterval(2000);
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

            QNetworkAccessManager nam;
            QNetworkRequest httpReq(QUrl("http://127.0.0.1:8000/api/qr-confirm"));
            httpReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QJsonDocument body(QJsonObject{{"session_id", sessionId}, {"username", testUser}});
            auto* reply = nam.post(httpReq, body.toJson());
            connect(reply, &QNetworkReply::finished, [&]() {
                auto respData = QJsonDocument::fromJson(reply->readAll());
                httpSuccess = respData.object()["success"].toBool();
                qDebug() << "POST /api/qr-confirm:" << respData.toJson(QJsonDocument::Compact);
                reply->deleteLater();
                loop.quit();
            });

            timeout.start();
            loop.exec();
        }

        QVERIFY2(httpSuccess, "HTTP POST to /api/qr-confirm failed");

        // 轮询等待 widget 状态更新（backend 推送 qr_scanned 后 widget 调用 showScanned）
        bool stateUpdated = false;
        for (int i = 0; i < 20; ++i) {  // 最多等待 2 秒
            QTest::qWait(100);
            QCoreApplication::processEvents();  // 强制处理事件队列
            if (m_loginWidget->qrStatusLabel->text() == "Scanned! Please confirm on your phone") {
                stateUpdated = true;
                break;
            }
        }

        QVERIFY2(stateUpdated, qPrintable(QString("Widget state not updated. Current label: '%1'")
                                          .arg(m_loginWidget->qrStatusLabel->text())));
        saveScreenshot("qr_scanned.png");
    }

    // ── Test: 有效 token 直接登录 ──────────────────────────────────
    void test_direct_login() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test: Direct Login (Token Verification) ===";

        QString token = getUserToken("screen_test_direct", "pass123");
        if (token.isEmpty()) QSKIP("Failed to obtain token");
        qDebug() << "✅ Got token, verifying...";

        bool tokenValid = false;
        {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.setInterval(1500);
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

            auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                    [&](const QString& type, const QJsonObject& data) {
                if (type == "verify_token") {
                    tokenValid = data["success"].toBool();
                    loop.quit();
                }
            });

            QJsonObject req;
            req["type"] = "verify_token";
            req["data"] = QJsonObject{{"token", token}};
            m_wsClient->send(req);

            timeout.start();
            loop.exec();
            disconnect(conn);
        }

        QVERIFY2(tokenValid, "Token verification failed");
        // 验证 widget 已进入 DirectLogin 状态（通过 friend access）
        // isVisible() 要求整个 widget 层级都已 show，此处 widget 尚未显示
        // isHidden() 只检查 widget 自身的隐藏状态，不依赖父级可见性
        QVERIFY(!m_loginWidget->directLoginButton->isHidden());
        saveScreenshot("direct_login.png");
    }

private:
    wechat::login::LoginWidget* m_loginWidget{nullptr};
    wechat::network::WsClient* m_wsClient{nullptr};
    bool m_connected{false};

    // 注册或登录用户，返回 token（失败返回空串）
    QString getUserToken(const QString& username, const QString& password) {
        for (const QString& msgType : {"register", "login"}) {
            QString token;
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            timeout.setInterval(1500);
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

            auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                    [&](const QString& type, const QJsonObject& data) {
                if (type == msgType) {
                    if (data["success"].toBool())
                        token = data["user"].toObject()["token"].toString();
                    loop.quit();
                }
            });

            QJsonObject req;
            req["type"] = msgType;
            req["data"] = QJsonObject{{"username", username}, {"password", password}};
            m_wsClient->send(req);

            timeout.start();
            loop.exec();
            disconnect(conn);

            if (!token.isEmpty()) return token;
        }
        return {};
    }

    void saveScreenshot(const QString& filename) {
        m_loginWidget->show();
        (void)QTest::qWaitForWindowExposed(m_loginWidget);
        QTest::qWait(100);
        QPixmap pix = m_loginWidget->grab();
        pix.toImage().save(filename, "PNG", 70);
        qDebug() << "📸 Screenshot saved:" << filename << "Size:" << pix.size();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    LoginScreenTest test;
    int result = QTest::qExec(&test, argc, argv);
    qDebug() << "\n========================================";
    qDebug() << "Screenshots saved to:" << QDir::currentPath();
    qDebug() << "========================================\n";
    return result;
}

#include "login_screen_test.moc"
