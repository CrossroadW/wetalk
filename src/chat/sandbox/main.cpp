#include <QApplication>
#include <spdlog/spdlog.h>

#include <wechat/log/Log.h>
#include <wechat/core/User.h>
#include <wechat/chat/ChatPresenter.h>
#include <wechat/network/NetworkClient.h>

#include "../ChatWidget.h"
#include "../MockAutoResponder.h"

#include <memory>

using namespace wechat;

int main(int argc, char* argv[]) {
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
    auto groupResult =
        networkClient->groups().createGroup(tokenAlice, {aliceId, bobId});
    auto chatId = groupResult.value().id;

    // 4. 创建 ChatPresenter（MVP 唯一中间层）
    chat::ChatPresenter presenter(*networkClient);
    presenter.setSession(tokenAlice, aliceId);

    // 5. 创建 ChatWidget（View）并注入 presenter
    chat::ChatWidget chatWidget;
    chatWidget.setCurrentUser(core::User{aliceId});
    chatWidget.setChatPartner(core::User{bobId});
    chatWidget.setPresenter(&presenter);

    // 6. 模拟对方用户（Bob）发消息
    chat::MockAutoResponder responder(*networkClient);
    responder.setResponderSession(tokenBob, bobId);
    responder.setChatId(chatId);

    // Bob 立即发几条消息（通过网络层推送通知自动同步到 UI）
    responder.sendMessage("Hey Alice! How are you?");
    responder.sendMessage("I just saw the news, pretty exciting!");

    // 7. 打开聊天（触发首次同步）
    presenter.openChat(chatId);

    chatWidget.show();

    spdlog::info("Chat sandbox started — alice <-> bob, signal-driven");
    return app.exec();
}
