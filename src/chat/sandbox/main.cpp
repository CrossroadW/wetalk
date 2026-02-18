#include <QApplication>
#include <spdlog/spdlog.h>

#include <wechat/log/Log.h>
#include <wechat/core/EventBus.h>
#include <wechat/core/User.h>
#include <wechat/chat/ChatManager.h>
#include <wechat/network/NetworkClient.h>

#include "../ChatWidget.h"
#include "../ChatController.h"
#include "../MockAutoResponder.h"

using namespace wechat;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    log::init();

    // 1. 创建 Mock 网络客户端
    auto networkClient = network::createMockClient();

    // 2. 注册两个用户
    auto regAlice = networkClient->auth().registerUser("alice", "pass");
    auto regBob = networkClient->auth().registerUser("bob", "pass");
    auto tokenAlice = regAlice.value().token;
    auto tokenBob = regBob.value().token;
    auto aliceId = regAlice.value().userId;
    auto bobId = regBob.value().userId;

    // 3. 建立好友关系并创建群聊
    networkClient->contacts().addFriend(tokenAlice, bobId);
    auto groupResult = networkClient->groups().createGroup(
        tokenAlice, {aliceId, bobId});
    auto chatId = groupResult.value().id;

    // 4. 创建 EventBus + ChatManager
    core::EventBus bus;
    chat::ChatManager manager(*networkClient, bus);
    manager.setSession(tokenAlice, aliceId);

    // 5. 创建 ChatController（Qt 桥接层）
    chat::ChatController controller(manager, bus);

    // 6. 创建 ChatWidget 并注入 controller
    chat::ChatWidget chatWidget;
    chatWidget.setCurrentUser(core::User{aliceId});
    chatWidget.setChatPartner(core::User{bobId});
    chatWidget.setController(&controller);

    // 7. 模拟对方用户（Bob）
    chat::MockAutoResponder responder(*networkClient);
    responder.setResponderSession(tokenBob, bobId);
    responder.setChatId(chatId);

    // Bob 预设几条消息，模拟聊天场景
    responder.scheduleMessage("Hey Alice! How are you?", 3000);
    responder.scheduleMessage("I just saw the news, pretty exciting!", 8000);

    // 8. 打开聊天并启动轮询
    controller.onOpenChat(QString::fromStdString(chatId));
    controller.startPolling(2000);  // 每 2 秒轮询一次

    chatWidget.show();

    spdlog::info("Chat sandbox started — alice <-> bob, polling every 2s");
    return app.exec();
}
