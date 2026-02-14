#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <wechat/log/Log.h>
#include "../ChatWidget.h"
#include <wechat/core/User.h>
#include <wechat/core/Message.h>

using namespace wechat;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // app.setStyleSheet(R"(*{border:1px solid green;})");
    log::init();
    // 创建聊天界面
    chat::ChatWidget chatWidget;

    // 设置模拟用户数据
    core::User currentUser;
    currentUser.id = "user1";

    core::User chatPartner;
    chatPartner.id = "user2";

    chatWidget.setCurrentUser(currentUser);
    chatWidget.setChatPartner(chatPartner);

    // 对方发送的普通多行文本消息（靠左）
    core::Message msg1;
    msg1.id = "msg1";
    msg1.senderId = "user2"; // 发送者是对方
    msg1.chatId = "chat_user1_user2";
    msg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 40000;
    msg1.updatedAt = 0;
    msg1.revoked = false;
    msg1.readCount = 0;
    msg1.replyTo = "";

    core::TextContent textContent1;
    textContent1.text = "Hello there! This is a longer message to demonstrate how text wraps and handles multiple lines in the message bubble. It contains more content to show how the layout adapts to different amounts of text.";
    msg1.content = {textContent1};

    chatWidget.getMessageListView()->addMessage(msg1, currentUser);

    // 对方发送的文本+图片消息（靠左）
    core::Message comboMsg1;
    comboMsg1.id = "combo_msg1";
    comboMsg1.senderId = "user2";
    comboMsg1.chatId = "chat_user1_user2";
    comboMsg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 30000;
    comboMsg1.updatedAt = 0;
    comboMsg1.revoked = false;
    comboMsg1.readCount = 0;
    comboMsg1.replyTo = "";

    core::TextContent comboText1;
    comboText1.text = "Look at this picture I took yesterday!";

    core::ResourceContent comboResource1;
    comboResource1.type = core::ResourceType::Image;
    comboResource1.subtype = core::ResourceSubtype::Png;
    comboResource1.resourceId = "image.png";
    comboResource1.meta.size = 0;
    comboResource1.meta.filename = "image.png";

    comboMsg1.content = {comboText1, comboResource1};

    chatWidget.getMessageListView()->addMessage(comboMsg1, currentUser);

    // 自己发送的普通多行文本消息（靠右）
    core::Message msg2;
    msg2.id = "msg2";
    msg2.senderId = "user1"; // 发送者是自己
    msg2.chatId = "chat_user1_user2";
    msg2.timestamp = QDateTime::currentMSecsSinceEpoch() - 20000;
    msg2.updatedAt = 0;
    msg2.revoked = false;
    msg2.readCount = 0;
    msg2.replyTo = "";

    core::TextContent textContent2;
    textContent2.text = "Hi! How are you? This is another longer message to show how text wraps and displays in the message bubble. The text should properly wrap to fit within the bubble.";
    msg2.content = {textContent2};

    chatWidget.getMessageListView()->addMessage(msg2, currentUser);

    // 自己发送的文本+图片消息（靠右）
    core::Message comboMsg2;
    comboMsg2.id = "combo_msg2";
    comboMsg2.senderId = "user1";
    comboMsg2.chatId = "chat_user1_user2";
    comboMsg2.timestamp = QDateTime::currentMSecsSinceEpoch() - 10000;
    comboMsg2.updatedAt = 0;
    comboMsg2.revoked = false;
    comboMsg2.readCount = 0;
    comboMsg2.replyTo = "";

    core::ResourceContent comboResource2;
    comboResource2.type = core::ResourceType::Image;
    comboResource2.subtype = core::ResourceSubtype::Png;
    comboResource2.resourceId = "image.png";
    comboResource2.meta.size = 0;
    comboResource2.meta.filename = "image.png";

    core::TextContent comboText2;
    comboText2.text = "Nice! Here's one from my perspective.";

    comboMsg2.content = {comboResource2, comboText2};

    chatWidget.getMessageListView()->addMessage(comboMsg2, currentUser);

    chatWidget.show();

    spdlog::info("Chat sandbox started with message list view");
    return app.exec();
}