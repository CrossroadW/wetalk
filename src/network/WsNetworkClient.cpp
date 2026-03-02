#include "WsNetworkClient.h"
#include "WsAuthService.h"
#include "WsChatService.h"
#include "WsContactService.h"
#include "WsGroupService.h"
#include "WsMomentService.h"
#include "WsClient.h"

#include <QEventLoop>
#include <QTimer>

namespace wechat {
namespace network {

WsNetworkClient::WsNetworkClient(const QString& wsUrl, int connectTimeout)
    : ws(std::make_unique<WsClient>()) {

    // 创建所有服务
    authService = std::make_unique<WsAuthService>(*ws);
    chatService = std::make_unique<WsChatService>(*ws, nullptr);
    contactService = std::make_unique<WsContactService>(*ws);
    groupService = std::make_unique<WsGroupService>(*ws);
    momentService = std::make_unique<WsMomentService>(*ws);

    // 连接到服务器
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(connectTimeout);

    bool connected = false;

    QObject::connect(ws.get(), &WebSocketClient::connected, [&]() {
        connected = true;
        loop.quit();
    });

    QObject::connect(ws.get(), &WebSocketClient::error, [&](const QString&) {
        loop.quit();
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    ws->connectToServer(wsUrl);
    timer.start();
    loop.exec();

    if (!connected) {
        throw std::runtime_error("Failed to connect to WebSocket server: " + wsUrl.toStdString());
    }
}

WsNetworkClient::~WsNetworkClient() = default;

AuthService& WsNetworkClient::auth() {
    return *authService;
}

ChatService& WsNetworkClient::chat() {
    return *chatService;
}

ContactService& WsNetworkClient::contacts() {
    return *contactService;
}

GroupService& WsNetworkClient::groups() {
    return *groupService;
}

MomentService& WsNetworkClient::moments() {
    return *momentService;
}

bool WsNetworkClient::isConnected() const {
    return ws->isConnected();
}

std::unique_ptr<NetworkClient> createWsClient(const QString& wsUrl) {
    return std::make_unique<WsNetworkClient>(wsUrl);
}

} // namespace network
} // namespace wechat
