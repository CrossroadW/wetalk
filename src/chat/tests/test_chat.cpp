#include <gtest/gtest.h>

#include <wechat/core/User.h>
#include <wechat/core/Message.h>

// 注意：GUI 组件（如 ChatWidget, MessageItemWidget）不适合单元测试，
// 因为它们需要图形界面环境才能正确运行。
// 因此，我们将测试集中在非 GUI 的逻辑组件上。

namespace wechat {
namespace chat {

TEST(ChatModuleTest, MessageStructure) {
    // 测试 Message 结构的基本功能
    core::Message msg;
    msg.id = "test_msg_1";
    msg.senderId = "user1";
    msg.chatId = "chat1";
    msg.timestamp = 1234567890;

    EXPECT_EQ(msg.id, "test_msg_1");
    EXPECT_EQ(msg.senderId, "user1");
    EXPECT_EQ(msg.chatId, "chat1");
    EXPECT_EQ(msg.timestamp, 1234567890);
}

TEST(ChatModuleTest, UserStructure) {
    // 测试 User 结构的基本功能
    core::User user;
    user.id = "test_user_1";

    EXPECT_EQ(user.id, "test_user_1");
}

TEST(ChatModuleTest, MessageContent) {
    // 测试消息内容结构
    core::TextContent textContent;
    textContent.text = "Hello, world!";

    EXPECT_EQ(textContent.text, "Hello, world!");
}

} // namespace chat
} // namespace wechat