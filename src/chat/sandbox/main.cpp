#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <wechat/log/Log.h>
#include "../ChatWidget.h"
#include <wechat/core/User.h>
#include <wechat/core/Message.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    wechat::log::init();
    // 创建聊天界面
    wechat::chat::ChatWidget chatWidget;

    // 设置模拟用户数据
    wechat::core::User currentUser;
    currentUser.id = "user1";

    wechat::core::User chatPartner;
    chatPartner.id = "user2";

    chatWidget.setCurrentUser(currentUser);
    chatWidget.setChatPartner(chatPartner);

    // 添加一些示例消息 - 对方发来的消息（靠左）
    wechat::core::Message msg1;
    msg1.id = "msg1";
    msg1.senderId = "user2";  // 发送者是对方
    msg1.chatId = "chat_user1_user2";
    msg1.timestamp = QDateTime::currentMSecsSinceEpoch() - 30000;
    msg1.updatedAt = 0;
    msg1.revoked = false;
    msg1.readCount = 0;
    msg1.replyTo = "";

    // 设置消息内容
    wechat::core::TextContent textContent1;
    textContent1.text = "Hello there!";
    msg1.content = {textContent1};

    chatWidget.getMessageListView()->addMessage(msg1, currentUser);

    // 自己发送的消息（靠右）
    wechat::core::Message msg2;
    msg2.id = "msg2";
    msg2.senderId = "user1";  // 发送者是自己
    msg2.chatId = "chat_user1_user2";
    msg2.timestamp = QDateTime::currentMSecsSinceEpoch() - 20000;
    msg2.updatedAt = 0;
    msg2.revoked = false;
    msg2.readCount = 0;
    msg2.replyTo = "";

    // 设置消息内容
    wechat::core::TextContent textContent2;
    textContent2.text = "Hi! How are you?";
    msg2.content = {textContent2};

    chatWidget.getMessageListView()->addMessage(msg2, currentUser);

    // 添加一张图片消息 - 对方发送（靠左）
    wechat::core::Message imgMsg;
    imgMsg.id = "img_msg1";
    imgMsg.senderId = "user2";  // 发送者是对方
    imgMsg.chatId = "chat_user1_user2";
    imgMsg.timestamp = QDateTime::currentMSecsSinceEpoch() - 10000;
    imgMsg.updatedAt = 0;
    imgMsg.revoked = false;
    imgMsg.readCount = 0;
    imgMsg.replyTo = "";

    wechat::core::TextContent content = {"teststststs"};
    // 设置图片消息内容
    wechat::core::ResourceContent resourceContent;
    resourceContent.type = wechat::core::ResourceType::Image;
    resourceContent.subtype = wechat::core::ResourceSubtype::Png;  // Specify PNG subtype
    resourceContent.resourceId = "image.png"; // Reference image in project's img directory
    resourceContent.meta.size = 0; // Will be determined when the file is accessed
    resourceContent.meta.filename = "image.png"; // Original filename

    imgMsg.content = {content,resourceContent};

    chatWidget.getMessageListView()->addMessage(imgMsg, currentUser);

    // 添加一张自己发送的图片消息（靠右）
    wechat::core::Message myImgMsg;
    myImgMsg.id = "my_img_msg1";
    myImgMsg.senderId = "user1";  // 发送者是自己
    myImgMsg.chatId = "chat_user1_user2";
    myImgMsg.timestamp = QDateTime::currentMSecsSinceEpoch() - 5000;
    myImgMsg.updatedAt = 0;
    myImgMsg.revoked = false;
    myImgMsg.readCount = 0;
    myImgMsg.replyTo = "";

    // 设置图片消息内容
    wechat::core::ResourceContent myResourceContent;
    myResourceContent.type = wechat::core::ResourceType::Image;
    myResourceContent.subtype = wechat::core::ResourceSubtype::Png;  // Specify PNG subtype
    myResourceContent.resourceId = "image.png"; // Reference image in project's img directory
    myResourceContent.meta.size = 0; // Will be determined when the file is accessed
    myResourceContent.meta.filename = "image.png"; // Original filename

    myImgMsg.content = {myResourceContent,content};

    chatWidget.getMessageListView()->addMessage(myImgMsg, currentUser);

    chatWidget.show();

    spdlog::info("Chat sandbox started with message list view");
    return app.exec();
}
