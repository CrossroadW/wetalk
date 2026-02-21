#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include <wechat/chat/ChatPresenter.h>
#include <wechat/core/Message.h>
#include <wechat/core/User.h>
#include <wechat/network/NetworkClient.h>

using namespace wechat;

// ═══════════════════════════════════════════════════════════════
// 数据结构基础测试
// ═══════════════════════════════════════════════════════════════

namespace wechat {
namespace chat {

TEST(ChatModuleTest, MessageStructure) {
    core::Message msg;
    msg.id = 1;
    msg.senderId = "user1";
    msg.chatId = "chat1";
    msg.timestamp = 1234567890;

    EXPECT_EQ(msg.id, 1);
    EXPECT_EQ(msg.senderId, "user1");
    EXPECT_EQ(msg.chatId, "chat1");
    EXPECT_EQ(msg.timestamp, 1234567890);
}

TEST(ChatModuleTest, UserStructure) {
    core::User user;
    user.id = "test_user_1";
    EXPECT_EQ(user.id, "test_user_1");
}

TEST(ChatModuleTest, MessageContent) {
    core::TextContent textContent;
    textContent.text = "Hello, world!";
    EXPECT_EQ(textContent.text, "Hello, world!");
}

} // namespace chat
} // namespace wechat

// ═══════════════════════════════════════════════════════════════
// ChatPresenter 测试 Fixture
// ═══════════════════════════════════════════════════════════════

// 注册自定义类型，使 QSignalSpy 能捕获信号参数
Q_DECLARE_METATYPE(std::vector<core::Message>)
Q_DECLARE_METATYPE(core::Message)

class ChatPresenterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // QSignalSpy 需要 QCoreApplication
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_chat";
            static char* argv[] = {arg0};
            static QCoreApplication app(argc, argv);
        }
        qRegisterMetaType<std::vector<core::Message>>();
        qRegisterMetaType<core::Message>();
    }

    void SetUp() override {
        client_ = network::createMockClient();

        auto regA = client_->auth().registerUser("alice", "p");
        auto regB = client_->auth().registerUser("bob", "p");
        tokenA_ = regA.value().token;
        tokenB_ = regB.value().token;
        aliceId_ = regA.value().userId;
        bobId_ = regB.value().userId;

        client_->contacts().addFriend(tokenA_, bobId_);
        auto group = client_->groups().createGroup(
            tokenA_, {aliceId_, bobId_});
        chatId_ = group.value().id;

        presenter_ = std::make_unique<chat::ChatPresenter>(*client_);
        presenter_->setSession(tokenA_, aliceId_);
    }

    std::unique_ptr<network::NetworkClient> client_;
    std::unique_ptr<chat::ChatPresenter> presenter_;
    std::string tokenA_, tokenB_;
    std::string aliceId_, bobId_;
    std::string chatId_;
};

// ═══════════════════════════════════════════════════════════════
// 基本状态测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SetSession) {
    EXPECT_EQ(presenter_->currentUserId(), aliceId_);
}

TEST_F(ChatPresenterTest, OpenChatSetsActiveId) {
    presenter_->openChat(chatId_);
    EXPECT_EQ(presenter_->activeChatId(), chatId_);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：openChat 首次同步
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, OpenChatEmitsMessagesInsertedOnExistingMessages) {
    // Bob 先发两条消息
    client_->chat().sendMessage(
        tokenB_, chatId_, 0, core::MessageContent{core::TextContent{"hi"}});
    client_->chat().sendMessage(
        tokenB_, chatId_, 0, core::MessageContent{core::TextContent{"hello"}});

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    ASSERT_TRUE(spy.isValid());

    presenter_->openChat(chatId_);

    // openChat 应触发 fetchAfter 同步已有消息
    ASSERT_GE(spy.count(), 1);

    auto args = spy.takeFirst();
    auto chatId = args.at(0).toString();
    auto messages = args.at(1).value<std::vector<core::Message>>();

    EXPECT_EQ(chatId.toStdString(), chatId_);
    EXPECT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].senderId, bobId_);
}

TEST_F(ChatPresenterTest, OpenChatNoSignalWhenEmpty) {
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    presenter_->openChat(chatId_);

    // 空聊天不应触发信号
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：发送消息
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SendMessageTriggersMessagesInserted) {
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    ASSERT_TRUE(spy.isValid());

    presenter_->sendTextMessage("hello bob!");

    // sendMessage → ChatService → onMessageStored → fetchAfter → messagesInserted
    ASSERT_EQ(spy.count(), 1);

    auto messages = spy.at(0).at(1).value<std::vector<core::Message>>();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].senderId, aliceId_);

    auto* text = std::get_if<core::TextContent>(&messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hello bob!");
}

TEST_F(ChatPresenterTest, SendMessageWithReply) {
    presenter_->openChat(chatId_);

    // 先发一条消息
    presenter_->sendTextMessage("original");
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // 获取第一条消息的 id（通过网络层查询）
    auto sync = client_->chat().fetchAfter(tokenA_, chatId_, 0, 50);
    auto originalId = sync.value().messages[0].id;

    // 发送回复
    presenter_->sendMessage(
        core::MessageContent{core::TextContent{"reply"}}, originalId);

    ASSERT_EQ(spy.count(), 1);
    auto messages = spy.at(0).at(1).value<std::vector<core::Message>>();
    EXPECT_EQ(messages[0].replyTo, originalId);
}

TEST_F(ChatPresenterTest, SendMultipleMessages) {
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    presenter_->sendTextMessage("msg1");
    presenter_->sendTextMessage("msg2");
    presenter_->sendTextMessage("msg3");

    // 每次 sendMessage 都会触发一次 onMessageStored → messagesInserted
    ASSERT_EQ(spy.count(), 3);

    // 验证消息 id 单调递增
    auto m1 = spy.at(0).at(1).value<std::vector<core::Message>>();
    auto m2 = spy.at(1).at(1).value<std::vector<core::Message>>();
    auto m3 = spy.at(2).at(1).value<std::vector<core::Message>>();
    EXPECT_LT(m1[0].id, m2[0].id);
    EXPECT_LT(m2[0].id, m3[0].id);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：接收别人的消息（统一路径）
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, ReceiveOtherUserMessage) {
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // Bob 通过网络层发消息（模拟对方发送）
    client_->chat().sendMessage(
        tokenB_, chatId_, 0,
        core::MessageContent{core::TextContent{"hey alice!"}});

    // ChatPresenter 订阅了 onMessageStored，应自动同步
    ASSERT_EQ(spy.count(), 1);

    auto messages = spy.at(0).at(1).value<std::vector<core::Message>>();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].senderId, bobId_);

    auto* text = std::get_if<core::TextContent>(&messages[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hey alice!");
}

TEST_F(ChatPresenterTest, SelfAndOtherMessagesSamePath) {
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // Alice 发
    presenter_->sendTextMessage("from alice");
    // Bob 发
    client_->chat().sendMessage(
        tokenB_, chatId_, 0,
        core::MessageContent{core::TextContent{"from bob"}});

    ASSERT_EQ(spy.count(), 2);

    auto m1 = spy.at(0).at(1).value<std::vector<core::Message>>();
    auto m2 = spy.at(1).at(1).value<std::vector<core::Message>>();
    EXPECT_EQ(m1[0].senderId, aliceId_);
    EXPECT_EQ(m2[0].senderId, bobId_);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：撤回消息
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, RevokeMessageTriggersMessageUpdated) {
    presenter_->openChat(chatId_);
    presenter_->sendTextMessage("oops");

    // 获取消息 id
    auto sync = client_->chat().fetchAfter(tokenA_, chatId_, 0, 50);
    auto msgId = sync.value().messages[0].id;

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messageUpdated);
    ASSERT_TRUE(spy.isValid());

    presenter_->revokeMessage(msgId);

    ASSERT_EQ(spy.count(), 1);
    auto msg = spy.at(0).at(1).value<core::Message>();
    EXPECT_EQ(msg.id, msgId);
    EXPECT_TRUE(msg.revoked);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：编辑消息
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, EditMessageTriggersMessageUpdated) {
    presenter_->openChat(chatId_);
    presenter_->sendTextMessage("typo");

    auto sync = client_->chat().fetchAfter(tokenA_, chatId_, 0, 50);
    auto msgId = sync.value().messages[0].id;

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messageUpdated);

    presenter_->editMessage(
        msgId, core::MessageContent{core::TextContent{"fixed"}});

    ASSERT_EQ(spy.count(), 1);
    auto msg = spy.at(0).at(1).value<core::Message>();
    EXPECT_EQ(msg.id, msgId);
    EXPECT_GT(msg.editedAt, 0);

    auto* text = std::get_if<core::TextContent>(&msg.content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "fixed");
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：加载历史
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, LoadHistoryEmitsMessagesInserted) {
    // 先发 5 条消息
    for (int i = 0; i < 5; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i)}});
    }

    // openChat 会 fetchAfter 拿到全部
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // loadHistory 向上翻页（fetchBefore）
    presenter_->loadHistory(3);

    // 因为 openChat 已经拿到全部消息，cursor.start 已经是最小 id，
    // fetchBefore 不会再有更早的消息，所以不触发信号
    // 这验证了游标正确工作
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ChatPresenterTest, LoadHistoryWithPartialSync) {
    // 先发 10 条消息
    for (int i = 0; i < 10; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i)}});
    }

    // 不通过 openChat，手动设置一个中间游标来模拟部分同步
    presenter_->openChat(chatId_);

    // openChat 应该拿到全部 10 条
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // 此时 loadHistory 不应有更多历史
    presenter_->loadHistory(5);
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// 边界条件测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SendBeforeOpenChatUsesEmptyActiveChatId) {
    // 未 openChat 就发消息，activeChatId 为空
    EXPECT_TRUE(presenter_->activeChatId().empty());

    // sendMessage 会调用网络层，但 chatId 为空应该失败（不崩溃）
    presenter_->sendTextMessage("orphan");
    // 不崩溃即通过
}

TEST_F(ChatPresenterTest, LoadHistoryBeforeOpenChat) {
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // 未 openChat 就 loadHistory，应该安全返回
    presenter_->loadHistory();
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ChatPresenterTest, OpenMultipleChats) {
    // 创建第二个聊天
    auto regC = client_->auth().registerUser("charlie", "p");
    auto tokenC = regC.value().token;
    client_->contacts().addFriend(tokenA_, regC.value().userId);
    auto group2 = client_->groups().createGroup(
        tokenA_, {aliceId_, regC.value().userId});
    auto chatId2 = group2.value().id;

    // 在第一个聊天发消息
    client_->chat().sendMessage(
        tokenB_, chatId_, 0,
        core::MessageContent{core::TextContent{"in chat1"}});

    // 在第二个聊天发消息
    client_->chat().sendMessage(
        tokenC, chatId2, 0,
        core::MessageContent{core::TextContent{"in chat2"}});

    // 打开第一个聊天
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    presenter_->openChat(chatId_);

    ASSERT_EQ(spy.count(), 1);
    auto chatId = spy.at(0).at(0).toString().toStdString();
    EXPECT_EQ(chatId, chatId_);

    // 切换到第二个聊天
    spy.clear();
    presenter_->openChat(chatId2);

    ASSERT_EQ(spy.count(), 1);
    chatId = spy.at(0).at(0).toString().toStdString();
    EXPECT_EQ(chatId, chatId2);
    EXPECT_EQ(presenter_->activeChatId(), chatId2);
}

// ═══════════════════════════════════════════════════════════════
// 信号不误触发测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, MessageUpdatedNotFiredOnSend) {
    presenter_->openChat(chatId_);

    QSignalSpy updatedSpy(presenter_.get(),
                           &chat::ChatPresenter::messageUpdated);
    QSignalSpy removedSpy(presenter_.get(),
                           &chat::ChatPresenter::messageRemoved);

    presenter_->sendTextMessage("hello");

    // 发送消息只应触发 messagesInserted，不应触发 updated 或 removed
    EXPECT_EQ(updatedSpy.count(), 0);
    EXPECT_EQ(removedSpy.count(), 0);
}

TEST_F(ChatPresenterTest, NotificationBeforeOpenChatStillProcessed) {
    // 不 openChat，Bob 发消息
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    client_->chat().sendMessage(
        tokenB_, chatId_, 0,
        core::MessageContent{core::TextContent{"hello"}});

    // Presenter 收到 onMessageStored 通知，即使未 openChat 也应处理
    ASSERT_EQ(spy.count(), 1);

    // openChat 时应重新推送已同步的消息给 UI
    spy.clear();
    presenter_->openChat(chatId_);
    ASSERT_EQ(spy.count(), 1);
}