#include "WsAuthService.h"

#include <QEventLoop>
#include <QFutureWatcher>
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

    if (!ws.isConnected()) return std::nullopt;

    // 将字段合并进 payload，由 WsClient.request() 自动附加 req_id
    QJsonObject payload;
    payload["type"] = requestType;
    for (auto it = requestData.constBegin(); it != requestData.constEnd(); ++it)
        payload[it.key()] = it.value();

    auto future = ws.request(payload);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout);

    QFutureWatcher<Result<QJsonObject>> watcher;
    QObject::connect(&watcher, &QFutureWatcherBase::finished,
                     &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout,
                     &loop, &QEventLoop::quit);

    watcher.setFuture(future);
    timer.start();
    loop.exec();

    if (!future.isFinished() || !future.result().has_value())
        return std::nullopt;
    return future.result().value();
}

} // namespace network
} // namespace wechat
