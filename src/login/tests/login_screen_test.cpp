#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QJsonObject>
#include <QEventLoop>

#include "../../../src/network/WsClient.h"
#include "../LoginWidget.h"

// 截图测试 — 使用真实 WebSocket 连接后端
// 要求：后端运行在 ws://localhost:8000/ws
class LoginScreenTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
        QDir rootDir(PROJECT_ROOT_PATH);
        QString outputPath = rootDir.filePath("tests");
        outputPath = QDir(outputPath).filePath("screenshots");
        outputPath = QDir(outputPath).filePath("login");
        outputPath = QDir(outputPath).filePath("output");

        QDir outputDir(outputPath);
        if (!outputDir.exists()) outputDir.mkpath(".");
        QDir::setCurrent(outputDir.absolutePath());
        qDebug() << "Output directory:" << QDir::currentPath();
        qDebug() << "⚠️  This test requires backend running at ws://localhost:8000/ws";
    }

    void init() {
        m_wsClient = new wechat::network::WsClient(this);
        m_loginWidget = new wechat::login::LoginWidget(m_wsClient);
        m_loginWidget->setObjectName("LoginWidget");
        m_widgetExposed = false;
    }

    void cleanup() {
        delete m_loginWidget;
        m_wsClient = nullptr;
    }

    void test01_qr_code_initial() {
        // 场景：首次启动，连接后端，获取二维码
        qDebug() << "\n=== Test 01: QR Code Initial ===";

        QEventLoop loop;
        bool connected = false;
        bool gotResponse = false;

        connect(m_wsClient, &wechat::network::WebSocketClient::connected, [&]() {
            qDebug() << "✅ WebSocket connected";
            connected = true;
        });

        connect(m_wsClient, &wechat::network::WebSocketClient::messageReceived,
                [&](const QString& type, const QJsonObject& data) {
            if (type == "qr_login_init") {
                qDebug() << "✅ Received qr_login_init response";
                qDebug() << "   session_id:" << data["session_id"].toString();
                qDebug() << "   qr_url:" << data["qr_url"].toString();
                gotResponse = true;
                loop.quit();
            }
        });

        connect(m_wsClient, &wechat::network::WebSocketClient::error,
                [&](const QString& err) {
            qWarning() << "❌ WebSocket error:" << err;
            loop.quit();
        });

        // 连接到后端
        m_wsClient->connectToServer("ws://localhost:8000/ws");

        // 等待连接
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();

        if (!connected) {
            QSKIP("Backend not available at ws://localhost:8000/ws");
        }

        // 发送 qr_login_init 请求
        QJsonObject req;
        req["type"] = "qr_login_init";
        req["data"] = QJsonObject{};
        m_wsClient->send(req);

        // 等待响应
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();

        QVERIFY2(gotResponse, "Did not receive qr_login_init response");

        saveScreenshot("01_qr_code_initial.png");
    }

    void test02_qr_code_loading() {
        // 场景：连接中，显示加载状态
        qDebug() << "\n=== Test 02: QR Code Loading ===";

        m_wsClient->connectToServer("ws://localhost:8000/ws");
        QTest::qWait(50);  // 连接中状态

        saveScreenshot("02_qr_code_loading.png");
    }

    void test04_direct_login() {
        // 场景：验证 token（需要数据库中有有效 token）
        qDebug() << "\n=== Test 04: Direct Login (Token Verification) ===";
        qDebug() << "⚠️  Skipped: Requires valid token in database";
        QSKIP("Requires valid token in database");
    }

private:
    wechat::login::LoginWidget* m_loginWidget{nullptr};
    wechat::network::WsClient* m_wsClient{nullptr};
    bool m_widgetExposed{false};

    void ensureWidgetShown() {
        if (!m_widgetExposed) {
            m_loginWidget->show();
            QTest::qWaitForWindowExposed(m_loginWidget);
            m_widgetExposed = true;
        }
    }

    void saveScreenshot(const QString& filename) {
        ensureWidgetShown();
        QTest::qWait(200);
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
