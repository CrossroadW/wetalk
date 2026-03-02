# WebSocket 客户端实现设计

## 概述

后端已从 HTTP REST API 切换到 WebSocket，所有通信都通过 WebSocket 进行。这份文档说明 C++ 客户端如何实现 WebSocket 通信。

**API 规范**: 详见 [../api-spec/](../api-spec/) 目录，包含完整的消息格式定义。

## 架构

```
┌─────────────────────────────────────────────┐
│  Qt Widgets (UI 层)                         │
│  LoginWidget, ChatWidget, ContactsWidget    │
└─────────────────┬───────────────────────────┘
                  │ Qt signals/slots
┌─────────────────▼───────────────────────────┐
│  Presenters (业务逻辑层)                     │
│  LoginPresenter, ChatPresenter, etc.        │
└─────────────────┬───────────────────────────┘
                  │ 方法调用
┌─────────────────▼───────────────────────────┐
│  NetworkClient (WebSocket 客户端)           │
│  - WebSocketClient (Qt WebSocket)           │
│  - 消息序列化/反序列化                       │
│  - 自动重连                                  │
└─────────────────┬───────────────────────────┘
                  │ WebSocket
┌─────────────────▼───────────────────────────┐
│  Backend Server (FastAPI + WebSocket)       │
└─────────────────────────────────────────────┘
```

## Qt WebSocket 使用

### 1. 添加依赖

在 `CMakeLists.txt` 中添加:

```cmake
find_package(Qt6 REQUIRED COMPONENTS WebSockets)
target_link_libraries(wechat_network PUBLIC Qt6::WebSockets)
```

### 2. WebSocketClient 类设计

```cpp
// include/wechat/network/WebSocketClient.h
#pragma once
#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <functional>
#include <memory>

namespace wechat::network {

class WebSocketClient : public QObject {
    Q_OBJECT

public:
    explicit WebSocketClient(const QString& url, QObject* parent = nullptr);
    ~WebSocketClient() override;

    // 连接/断开
    void connect();
    void disconnect();
    bool isConnected() const;

    // 发送消息（异步，通过回调返回结果）
    void send(const QString& type, const QJsonObject& data,
              std::function<void(bool success, const QJsonObject& response)> callback);

    // 注册推送消息处理器
    void onPush(const QString& type, std::function<void(const QJsonObject& data)> handler);

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(const QString& errorString);

private Q_SLOTS:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace wechat::network
```

### 3. 消息格式

所有消息都是 JSON 格式:

**发送**:
```json
{
  "type": "message_type",
  "data": { ... }
}
```

**接收**:
```json
{
  "type": "message_type",
  "success": true/false,
  "data": { ... }
}
```

### 4. 实现示例

```cpp
// src/network/WebSocketClient.cpp
#include "wechat/network/WebSocketClient.h"
#include <QJsonDocument>
#include <QTimer>
#include <unordered_map>

namespace wechat::network {

struct WebSocketClient::Impl {
    QWebSocket socket;
    QString url;
    std::unordered_map<QString, std::function<void(const QJsonObject&)>> pushHandlers;
    std::queue<std::pair<int, std::function<void(bool, const QJsonObject&)>>> callbacks;
    int nextRequestId = 1;
    bool autoReconnect = true;
};

WebSocketClient::WebSocketClient(const QString& url, QObject* parent)
    : QObject(parent), pImpl(std::make_unique<Impl>()) {
    pImpl->url = url;

    QObject::connect(&pImpl->socket, &QWebSocket::connected,
                     this, &WebSocketClient::onConnected);
    QObject::connect(&pImpl->socket, &QWebSocket::disconnected,
                     this, &WebSocketClient::onDisconnected);
    QObject::connect(&pImpl->socket, &QWebSocket::textMessageReceived,
                     this, &WebSocketClient::onTextMessageReceived);
    QObject::connect(&pImpl->socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this, &WebSocketClient::onError);
}

void WebSocketClient::connect() {
    pImpl->socket.open(QUrl(pImpl->url));
}

void WebSocketClient::send(const QString& type, const QJsonObject& data,
                           std::function<void(bool, const QJsonObject&)> callback) {
    QJsonObject message;
    message["type"] = type;
    message["data"] = data;

    QJsonDocument doc(message);
    pImpl->socket.sendTextMessage(doc.toJson(QJsonDocument::Compact));

    // 保存回调
    int requestId = pImpl->nextRequestId++;
    pImpl->callbacks.push({requestId, callback});
}

void WebSocketClient::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();

    QString type = obj["type"].toString();
    bool success = obj["success"].toBool();
    QJsonObject data = obj["data"].toObject();

    // 检查是否是推送消息
    if (pImpl->pushHandlers.contains(type)) {
        pImpl->pushHandlers[type](data);
        return;
    }

    // 否则是请求响应
    if (!pImpl->callbacks.empty()) {
        auto [requestId, callback] = pImpl->callbacks.front();
        pImpl->callbacks.pop();
        callback(success, data);
    }
}

void WebSocketClient::onPush(const QString& type,
                             std::function<void(const QJsonObject&)> handler) {
    pImpl->pushHandlers[type] = handler;
}

} // namespace wechat::network
```

## 二维码登录实现

### 1. LoginPresenter 修改

```cpp
// include/wechat/login/LoginPresenter.h
class LoginPresenter : public QObject {
    Q_OBJECT

public:
    // 二维码登录
    void startQRLogin();  // 生成二维码
    void verifyToken(const std::string& token);  // 验证已保存的 token

Q_SIGNALS:
    void qrCodeGenerated(const QString& qrUrl, int expiresAt);
    void loginSuccess(const User& user, const std::string& token);
    void loginFailed(const QString& reason);

private:
    WebSocketClient* wsClient;
};
```

### 2. 实现流程

```cpp
void LoginPresenter::startQRLogin() {
    wsClient->send("qr_login_init", QJsonObject{}, [this](bool success, const QJsonObject& data) {
        if (success) {
            QString qrUrl = data["qr_url"].toString();
            int expiresAt = data["expires_at"].toInt();
            Q_EMIT qrCodeGenerated(qrUrl, expiresAt);
        }
    });

    // 注册推送处理器
    wsClient->onPush("qr_confirmed", [this](const QJsonObject& data) {
        int userId = data["user_id"].toInt();
        QString username = data["username"].toString();
        QString token = data["token"].toString();

        User user{userId, username.toStdString()};
        Q_EMIT loginSuccess(user, token.toStdString());
    });
}

void LoginPresenter::verifyToken(const std::string& token) {
    QJsonObject data;
    data["token"] = QString::fromStdString(token);

    wsClient->send("verify_token", data, [this, token](bool success, const QJsonObject& data) {
        if (success) {
            int userId = data["user_id"].toInt();
            QString username = data["username"].toString();

            User user{userId, username.toStdString()};
            Q_EMIT loginSuccess(user, token);
        } else {
            Q_EMIT loginFailed("Token 无效");
        }
    });
}
```

### 3. LoginWidget 修改

```cpp
// src/login/LoginWidget.cpp
void LoginWidget::setupUI() {
    // 检查是否有保存的 token
    QString savedToken = loadTokenFromFile();

    if (!savedToken.isEmpty()) {
        // 显示"进入微信"按钮
        showDirectLoginButton();
    } else {
        // 显示二维码
        showQRCode();
    }
}

void LoginWidget::showQRCode() {
    presenter->startQRLogin();
}

void LoginWidget::onQRCodeGenerated(const QString& qrUrl, int expiresAt) {
    // 使用 zxing 生成二维码图片
    QImage qrImage = generateQRCode(qrUrl);
    qrLabel->setPixmap(QPixmap::fromImage(qrImage));

    // 设置过期定时器
    QTimer::singleShot((expiresAt - currentTime()) * 1000, this, [this]() {
        // 二维码过期，重新生成
        showQRCode();
    });
}

void LoginWidget::onLoginSuccess(const User& user, const std::string& token) {
    // 保存 token
    saveTokenToFile(QString::fromStdString(token));

    // 发送登录成功信号
    Q_EMIT loginSuccess(user, token);
}
```

## Token 持久化

### 1. 保存位置

使用 `AppPaths` 获取数据目录:

```cpp
#include "wechat/core/AppPaths.h"

QString getTokenFilePath() {
    auto dataDir = wechat::core::AppPaths::instance().dataDir();
    return QString::fromStdString((dataDir / "token.txt").string());
}
```

### 2. 保存/读取

```cpp
void saveTokenToFile(const QString& token) {
    QString filePath = getTokenFilePath();
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << token;
        file.close();
    }
}

QString loadTokenFromFile() {
    QString filePath = getTokenFilePath();
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString token = in.readAll().trimmed();
        file.close();
        return token;
    }
    return QString();
}
```

## 二维码生成

使用 zxing-cpp 库生成二维码:

```cpp
#include <ZXing/BarcodeFormat.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/BitMatrix.h>

QImage generateQRCode(const QString& text, int size = 300) {
    using namespace ZXing;

    auto writer = MultiFormatWriter(BarcodeFormat::QRCode);
    auto matrix = writer.encode(text.toStdString(), size, size);

    QImage image(size, size, QImage::Format_RGB32);
    image.fill(Qt::white);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (matrix.get(x, y)) {
                image.setPixel(x, y, qRgb(0, 0, 0));
            }
        }
    }

    return image;
}
```

## 其他服务实现

### ChatService

```cpp
void ChatService::sendMessage(int64_t chatId, const std::string& contentData, int64_t replyTo) {
    QJsonObject data;
    data["chat_id"] = static_cast<qint64>(chatId);
    data["content_data"] = QString::fromStdString(contentData);
    data["reply_to"] = static_cast<qint64>(replyTo);

    wsClient->send("send_message", data, [this](bool success, const QJsonObject& response) {
        if (success) {
            // 解析消息
            QJsonObject msgObj = response["message"].toObject();
            Message msg = parseMessage(msgObj);
            Q_EMIT messageStored(msg);
        }
    });
}

void ChatService::fetchMessages(int64_t chatId, int64_t beforeId, int limit) {
    QJsonObject data;
    data["chat_id"] = static_cast<qint64>(chatId);
    data["before_id"] = static_cast<qint64>(beforeId);
    data["limit"] = limit;

    wsClient->send("get_messages", data, [this](bool success, const QJsonObject& response) {
        if (success) {
            QJsonArray messagesArray = response["messages"].toArray();
            std::vector<Message> messages;
            for (const auto& msgValue : messagesArray) {
                messages.push_back(parseMessage(msgValue.toObject()));
            }
            Q_EMIT messagesFetched(messages);
        }
    });
}
```

## 自动重连

```cpp
void WebSocketClient::onDisconnected() {
    Q_EMIT disconnected();

    if (pImpl->autoReconnect) {
        // 5 秒后重连
        QTimer::singleShot(5000, this, [this]() {
            connect();
        });
    }
}
```

## 总结

1. **使用 Qt WebSocket**: `QWebSocket` 类处理 WebSocket 连接
2. **JSON 消息**: 所有消息都是 JSON 格式，使用 `QJsonDocument` 序列化/反序列化
3. **异步回调**: 使用 `std::function` 回调处理响应
4. **推送通知**: 注册推送处理器接收服务器主动推送的消息
5. **二维码登录**: 使用 zxing 生成二维码，等待推送通知
6. **Token 持久化**: 保存到本地文件，下次启动时验证

## 下一步

1. 实现 `WebSocketClient` 类
2. 修改 `LoginPresenter` 和 `LoginWidget` 支持二维码登录
3. 更新所有 Service 类使用 WebSocket 而不是 HTTP
4. 添加 zxing 依赖到 Conan
5. 实现 token 持久化
