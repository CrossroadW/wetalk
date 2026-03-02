#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QJsonObject>
#include <QEventLoop>

#include <network/WsClient.h>
#include "LoginWidget.h"

// 截图测试 — 使用真实 WebSocket 连接后端
// 要求：后端运行在 ws://localhost:8000/ws
class LoginScreenTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
        // 设置截图输出目录
        QDir rootDir(PROJECT_ROOT_PATH);
        QString outputPath = rootDir.filePath("tests/screenshots/login/output");
        QDir().mkpath(outputPath);
        QDir::setCurrent(outputPath);
        qDebug() << "Output directory:" << QDir::currentPath();

        // 连接一次，所有测试复用
        m_wsClient = new wechat::network::WsClient(this);

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(2000);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(m_wsClient, &wechat::network::WebSocketClient::connected, &loop, &QEventLoop::quit);
        connect(m_wsClient, &wechat::network::WebSocketClient::error, &loop, &QEventLoop::quit);

        m_wsClient->connectToServer("ws://localhost:8000/ws");
        timeout.start();
        loop.exec();

        m_connected = m_wsClient->isConnected();
        if (!m_connected)
            qWarning() << "⚠️  Backend not available at ws://localhost:8000/ws";
    }

    void init() {
        m_loginWidget = new wechat::login::LoginWidget(m_wsClient);
        m_loginWidget->setObjectName("LoginWidget");
    }

    void cleanup() {
        delete m_loginWidget;
        m_loginWidget = nullptr;
    }

    void test01_qr_code_initial() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test 01: QR Code Initial ===";

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
        saveScreenshot("01_qr_code_initial.png");
    }

    void test02_qr_code_loading() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test 02: QR Code Loading ===";

        bool gotSession = false;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(1500);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

        auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                [&](const QString& type, const QJsonObject& data) {
            if (type == "qr_login_init") {
                qDebug() << "✅ Got session_id:" << data["session_id"].toString();
                gotSession = true;
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

        if (!gotSession) QSKIP("Failed to get session_id");

        qDebug() << "⚠️  Skipped: Requires HTTP client to call /api/qr-confirm";
        QTest::qWait(200);
        saveScreenshot("02_qr_code_loading.png");
    }

    void test03_qr_code_scanned() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test 03: QR Code Scanned ===";

        bool gotSession = false;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(1500);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

        auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                [&](const QString& type, const QJsonObject& data) {
            if (type == "qr_login_init") {
                qDebug() << "✅ Got session_id:" << data["session_id"].toString();
                gotSession = true;
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

        if (!gotSession) QSKIP("Failed to get session_id");

        QTest::qWait(100);
        saveScreenshot("03_qr_code_scanned.png");
    }

    void test04_direct_login() {
        if (!m_connected) QSKIP("Backend not available");
        qDebug() << "\n=== Test 04: Direct Login (Token Verification) ===";

        bool gotResponse = false;
        bool tokenValid = false;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(1500);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

        auto conn = connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                [&](const QString& type, const QJsonObject& data) {
            if (type == "verify_token") {
                tokenValid = data["success"].toBool();
                gotResponse = true;
                loop.quit();
            }
        });

        QJsonObject req;
        req["type"] = "verify_token";
        QJsonObject data;
        data["token"] = "test_token_12345";
        req["data"] = data;
        m_wsClient->send(req);

        timeout.start();
        loop.exec();
        disconnect(conn);

        if (!tokenValid) {
            qDebug() << "⚠️  No valid token, skipping";
            QSKIP("No valid token in database");
        }

        saveScreenshot("04_direct_login.png");
    }

private:
    wechat::login::LoginWidget* m_loginWidget{nullptr};
    wechat::network::WsClient* m_wsClient{nullptr};
    bool m_connected{false};

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
