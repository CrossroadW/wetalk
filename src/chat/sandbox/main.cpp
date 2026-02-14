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

    // 添加一些示例消息 - 对方发来的消息（靠左）
    core::Message msg1;
    msg1.id = "msg1";
    msg1.senderId = "user2"; // 发送者是对方
    msg1.chatId = "chat_user1_user2";
    msg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 60000;
    msg1.updatedAt = 0;
    msg1.revoked = false;
    msg1.readCount = 0;
    msg1.replyTo = "";

    // 设置消息内容
    core::TextContent textContent1;
    textContent1.text = "Hello there!";
    msg1.content = {textContent1};

    chatWidget.getMessageListView()->addMessage(msg1, currentUser);

    // 自己发送的消息（靠右）
    core::Message msg2;
    msg2.id = "msg2";
    msg2.senderId = "user1"; // 发送者是自己
    msg2.chatId = "chat_user1_user2";
    msg2.timestamp = QDateTime::currentMSecsSinceEpoch() - 50000;
    msg2.updatedAt = 0;
    msg2.revoked = false;
    msg2.readCount = 0;
    msg2.replyTo = "";

    // 设置消息内容
    core::TextContent textContent2;
    textContent2.text = "Hi! How are you?";
    msg2.content = {textContent2};

    chatWidget.getMessageListView()->addMessage(msg2, currentUser);

    // 第二条对方消息
    core::Message msg3;
    msg3.id = "msg3";
    msg3.senderId = "user2";
    msg3.chatId = "chat_user1_user2";
    msg3.timestamp = QDateTime::currentMSecsSinceEpoch() - 40000;
    msg3.updatedAt = 0;
    msg3.revoked = false;
    msg3.readCount = 0;
    msg3.replyTo = "";

    core::TextContent textContent3;
    textContent3.text = "I'm doing well, thanks for asking! Just finished a great project.";
    msg3.content = {textContent3};

    chatWidget.getMessageListView()->addMessage(msg3, currentUser);

    // 更长的文本消息 - 自己发送（靠右）
    core::Message msg4;
    msg4.id = "msg4";
    msg4.senderId = "user1";
    msg4.chatId = "chat_user1_user2";
    msg4.timestamp = QDateTime::currentMSecsSinceEpoch() - 30000;
    msg4.updatedAt = 0;
    msg4.revoked = false;
    msg4.readCount = 0;
    msg4.replyTo = "";

    core::TextContent textContent4;
    textContent4.text = "That's great to hear! I've been working on some new features for our application. The development is going smoothly, and I think users will really enjoy the improvements we're making.";
    msg4.content = {textContent4};

    chatWidget.getMessageListView()->addMessage(msg4, currentUser);

    // 图片消息 - 对方发送（靠左）
    core::Message imgMsg1;
    imgMsg1.id = "img_msg1";
    imgMsg1.senderId = "user2"; // 发送者是对方
    imgMsg1.chatId = "chat_user1_user2";
    imgMsg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 25000;
    imgMsg1.updatedAt = 0;
    imgMsg1.revoked = false;
    imgMsg1.readCount = 0;
    imgMsg1.replyTo = "";

    // 设置图片消息内容
    core::ResourceContent resourceContent1;
    resourceContent1.type = core::ResourceType::Image;
    resourceContent1.subtype = core::ResourceSubtype::Png; // Specify PNG subtype
    resourceContent1.resourceId = "image.png"; // Reference image in project's img directory
    resourceContent1.meta.size = 0;           // Will be determined when the file is accessed
    resourceContent1.meta.filename = "image.png"; // Original filename

    imgMsg1.content = {resourceContent1};

    chatWidget.getMessageListView()->addMessage(imgMsg1, currentUser);

    // 自己发送的图片消息（靠右）
    core::Message myImgMsg1;
    myImgMsg1.id = "my_img_msg1";
    myImgMsg1.senderId = "user1"; // 发送者是自己
    myImgMsg1.chatId = "chat_user1_user2";
    myImgMsg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 20000;
    myImgMsg1.updatedAt = 0;
    myImgMsg1.revoked = false;
    myImgMsg1.readCount = 0;
    myImgMsg1.replyTo = "";

    // 设置图片消息内容
    core::ResourceContent myResourceContent1;
    myResourceContent1.type = core::ResourceType::Image;
    myResourceContent1.subtype = core::ResourceSubtype::Png; // Specify PNG subtype
    myResourceContent1.resourceId = "image.png"; // Reference image in project's img directory
    myResourceContent1.meta.size = 0;           // Will be determined when the file is accessed
    myResourceContent1.meta.filename = "image.png"; // Original filename

    myImgMsg1.content = {myResourceContent1};

    chatWidget.getMessageListView()->addMessage(myImgMsg1, currentUser);

    // 文本+图片组合消息 - 对方发送
    core::Message comboMsg1;
    comboMsg1.id = "combo_msg1";
    comboMsg1.senderId = "user2";
    comboMsg1.chatId = "chat_user1_user2";
    comboMsg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 15000;
    comboMsg1.updatedAt = 0;
    comboMsg1.revoked = false;
    comboMsg1.readCount = 0;
    comboMsg1.replyTo = "";

    // 设置文本内容
    core::TextContent comboText1;
    comboText1.text = "Look at this picture I took yesterday!";

    // 设置图片内容
    core::ResourceContent comboResource1;
    comboResource1.type = core::ResourceType::Image;
    comboResource1.subtype = core::ResourceSubtype::Png;
    comboResource1.resourceId = "image.png";
    comboResource1.meta.size = 0;
    comboResource1.meta.filename = "image.png";

    comboMsg1.content = {comboText1, comboResource1};

    chatWidget.getMessageListView()->addMessage(comboMsg1, currentUser);

    // 图片+文本组合消息 - 自己发送（靠右）
    core::Message comboMsg2;
    comboMsg2.id = "combo_msg2";
    comboMsg2.senderId = "user1";
    comboMsg2.chatId = "chat_user1_user2";
    comboMsg2.timestamp = QDateTime::currentMSecsSinceEpoch() - 10000;
    comboMsg2.updatedAt = 0;
    comboMsg2.revoked = false;
    comboMsg2.readCount = 0;
    comboMsg2.replyTo = "";

    // 设置图片内容
    core::ResourceContent comboResource2;
    comboResource2.type = core::ResourceType::Image;
    comboResource2.subtype = core::ResourceSubtype::Png;
    comboResource2.resourceId = "image.png";
    comboResource2.meta.size = 0;
    comboResource2.meta.filename = "image.png";

    // 设置文本内容
    core::TextContent comboText2;
    comboText2.text = "Nice! Here's one from my perspective.";

    comboMsg2.content = {comboResource2, comboText2};

    chatWidget.getMessageListView()->addMessage(comboMsg2, currentUser);

    // 最后一条消息
    core::Message finalMsg;
    finalMsg.id = "final_msg1";
    finalMsg.senderId = "user2";
    finalMsg.chatId = "chat_user1_user2";
    finalMsg.timestamp = QDateTime::currentMSecsSinceEpoch() - 5000;
    finalMsg.updatedAt = 0;
    finalMsg.revoked = false;
    finalMsg.readCount = 0;
    finalMsg.replyTo = "";

    core::TextContent finalText;
    finalText.text = "Thanks for sharing! It's great to exchange ideas and photos with you.";
    finalMsg.content = {finalText};

    chatWidget.getMessageListView()->addMessage(finalMsg, currentUser);

    chatWidget.show();

    spdlog::info("Chat sandbox started with message list view");
    return app.exec();
}
