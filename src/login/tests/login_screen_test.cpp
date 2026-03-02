#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QJsonObject>

#include <wechat/network/WebSocketClient.h>
#include "../LoginWidget.h"

// Mock WebSocketClient — 模拟真实 WebSocket 服务器的行为
// 收到 send() 调用后，根据消息类型异步发射对应的 messageReceived 信号
class MockWebSocketClient : public wechat::network::WebSocketClient {
    Q_OBJECT
public:
    explicit MockWebSocketClient(QObject* parent = nullptr)
        : wechat::network::WebSocketClient(parent) {}

    void connectToServer(const QString& /*url*/) override {
        QTimer::singleShot(50, this, [this]() {
            Q_EMIT connected();
        });
    }

    void send(const QJsonObject& message) override {
        QString type = message["type"].toString();

        if (type == "qr_login_init") {
            QTimer::singleShot(100, this, [this]() {
                QJsonObject data;
                data["session_id"] = "session_123";
                data["qr_url"] = "http://localhost:8000/qr-login?session=session_123";
                data["expires_at"] = 1234567890;
                Q_EMIT messageReceived("qr_login_init", data);
            });
        } else if (type == "verify_token") {
            QString token = message["data"].toObject()["token"].toString();
            QTimer::singleShot(50, this, [this, token]() {
                QJsonObject data;
                if (token == "valid_token_123") {
                    data["success"] = true;
                    data["user_id"] = 1;
                    data["username"] = "Alice";
                } else {
                    data["success"] = false;
                    data["message"] = "invalid token";
                }
                Q_EMIT messageReceived("verify_token", data);
            });
        }
    }

    bool isConnected() const override { return true; }

    // 测试辅助：模拟服务器推送扫码事件
    void simulateQRScanned() {
        QTimer::singleShot(50, this, [this]() {
            Q_EMIT messageReceived("qr_scanned", QJsonObject{});
        });
    }

    // 测试辅助：模拟服务器推送确认事件
    void simulateQRConfirmed(int userId, const QString& username, const QString& token) {
        QTimer::singleShot(50, this, [this, userId, username, token]() {
            QJsonObject data;
            data["user_id"] = userId;
            data["username"] = username;
            data["token"] = token;
            Q_EMIT messageReceived("qr_confirmed", data);
        });
    }
};

// 测试类
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
    }

    void init() {
        m_wsClient = new MockWebSocketClient(this);
        m_loginWidget = new wechat::login::LoginWidget(m_wsClient);
        m_loginWidget->setObjectName("LoginWidget");
        m_widgetExposed = false;
    }

    void cleanup() {
        delete m_loginWidget;
        m_wsClient = nullptr;
    }

    void test01_qr_code_initial() {
        // 流程：connectToServer → connected → send(qr_login_init) → messageReceived → 显示二维码
        m_wsClient->connectToServer("ws://localhost:8000/ws");
        QTest::qWait(100);

        QJsonObject req;
        req["type"] = "qr_login_init";
        m_wsClient->send(req);
        QTest::qWait(200);

        saveScreenshot("01_qr_code_initial.png");
    }

    void test02_qr_code_loading() {
        // 场景：WebSocket 连接中，显示加载状态
        m_wsClient->connectToServer("ws://localhost:8000/ws");
        QTest::qWait(20);

        saveScreenshot("02_qr_code_loading.png");
    }

    void test03_qr_code_scanned() {
        // 场景：二维码已扫描，等待手机端确认
        m_wsClient->connectToServer("ws://localhost:8000/ws");
        QTest::qWait(100);

        QJsonObject req;
        req["type"] = "qr_login_init";
        m_wsClient->send(req);
        QTest::qWait(200);

        m_wsClient->simulateQRScanned();
        QTest::qWait(100);

        saveScreenshot("03_qr_code_scanned.png");
    }

    void test04_direct_login() {
        // 场景：本地有有效 token，显示直接登录按钮
        m_wsClient->connectToServer("ws://localhost:8000/ws");
        QTest::qWait(100);

        QJsonObject req;
        req["type"] = "verify_token";
        req["data"] = QJsonObject{{"token", "valid_token_123"}};
        m_wsClient->send(req);
        QTest::qWait(100);

        saveScreenshot("04_direct_login.png");
    }

private:
    wechat::login::LoginWidget* m_loginWidget{nullptr};
    MockWebSocketClient* m_wsClient{nullptr};
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
        QTest::qWait(100);
        QPixmap pix = m_loginWidget->grab();
        pix.toImage().save(filename, "PNG", 70);
        qDebug() << "Generated:" << filename << "Size:" << pix.size();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    LoginScreenTest test;
    int result = QTest::qExec(&test, argc, argv);
    qDebug() << "Screenshots saved to:" << QDir::currentPath();
    return result;
}

#include "login_screen_test.moc"
