#include "WsAuthService.h"

#include <optional>

#include <QEventLoop>
#include <QJsonDocument>
#include <QTimer>

namespace wechat {
namespace network {

WsAuthService::WsAuthService(WebSocketClient& ws)
    : ws(ws) {}

Result<core::User> WsAuthService::registerUser(
    const std::string& username, const std::string& password) {

    QJsonObject data;
    data["username"] = QString::fromStdString(username);
    data["password"] = QString::fromStdString(password);

    auto response = sendAndWait("register", data);
    if (!response.has_value()) {
        return std::unexpected("Backend connection failed or timeout");
    }

    auto resp = response.value();
    if (!resp["success"].toBool()) {
        return std::unexpected(resp["error"].toString().toStdString());
    }

    auto userData = resp["user"].toObject();
    core::User user;
    user.id = userData["id"].toInt();
    user.username = userData["username"].toString().toStdString();
    user.password = password;
    user.token = userData["token"].toString().toStdString();

    return user;
}

Result<core::User> WsAuthService::login(
    const std::string& username, const std::string& password) {

    QJsonObject data;
    data["username"] = QString::fromStdString(username);
    data["password"] = QString::fromStdString(password);

    auto response = sendAndWait("login", data);
    if (!response.has_value()) {
        return std::unexpected("Backend connection failed or timeout");
    }

    auto resp = response.value();
    if (!resp["success"].toBool()) {
        return std::unexpected(resp["error"].toString().toStdString());
    }

    auto userData = resp["user"].toObject();
    core::User user;
    user.id = userData["id"].toInt();
    user.username = userData["username"].toString().toStdString();
    user.password = password;
    user.token = userData["token"].toString().toStdString();

    return user;
}

VoidResult WsAuthService::logout(const std::string& token) {
    QJsonObject data;
    data["token"] = QString::fromStdString(token);

    auto response = sendAndWait("logout", data);
    if (!response.has_value()) {
        return std::unexpected("Backend connection failed or timeout");
    }

    auto resp = response.value();
    if (!resp["success"].toBool()) {
        return std::unexpected(resp["error"].toString().toStdString());
    }

    return {};
}

Result<core::User> WsAuthService::getCurrentUser(const std::string& token) {
    QJsonObject data;
    data["token"] = QString::fromStdString(token);

    auto response = sendAndWait("verify_token", data);
    if (!response.has_value()) {
        return std::unexpected("Backend connection failed or timeout");
    }

    auto resp = response.value();
    if (!resp["success"].toBool()) {
        return std::unexpected(resp["error"].toString().toStdString());
    }

    auto userData = resp["user"].toObject();
    core::User user;
    user.id = userData["id"].toInt();
    user.username = userData["username"].toString().toStdString();
    user.token = token;

    return user;
}

std::optional<QJsonObject> WsAuthService::sendAndWait(
    const QString& requestType,
    const QJsonObject& requestData,
    int timeout) {

    if (!ws.isConnected()) {
        return std::nullopt;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout);

    std::optional<QJsonObject> result;
    bool gotResponse = false;

    // 连接响应信号
    auto conn = QObject::connect(&ws, &WebSocketClient::messageReceived,
                       [&](const QString& type, const QJsonObject& data) {
        if (type == requestType) {
            result = data;
            gotResponse = true;
            loop.quit();
        }
    });

    // 连接超时信号
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // 发送请求
    QJsonObject request;
    request["type"] = requestType;
    request["data"] = requestData;
    ws.send(request);

    // 启动超时定时器
    timer.start();

    // 等待响应或超时
    loop.exec();

    // 断开连接
    QObject::disconnect(conn);

    return result;
}

} // namespace network
} // namespace wechat
