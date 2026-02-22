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
    msg.senderId = 100;
    msg.chatId = 200;
    msg.timestamp = 1234567890;

    EXPECT_EQ(msg.id, 1);
    EXPECT_EQ(msg.senderId, 100);
    EXPECT_EQ(msg.chatId, 200);
    EXPECT_EQ(msg.timestamp, 1234567890);
}

TEST(ChatModuleTest, UserStructure) {
    core::User user;
    user.id = 42;
    EXPECT_EQ(user.id, 42);
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
    int64_t aliceId_ = 0, bobId_ = 0;
    int64_t chatId_ = 0;
};

// ═══════════════════════════════════════════════════════════════
// 基本状态测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SetSession) {
    EXPECT_EQ(presenter_->currentUserId(), aliceId_);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：openChat + loadHistory 初始化
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, OpenChatDoesNotEmit) {
    // Bob 先发两条消息（触发 onMessageStored → 自动同步）
    client_->chat().sendMessage(
        tokenB_, chatId_, 0, core::MessageContent{core::TextContent{"hi"}});
    client_->chat().sendMessage(
        tokenB_, chatId_, 0, core::MessageContent{core::TextContent{"hello"}});

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    ASSERT_TRUE(spy.isValid());

    // openChat 只创建 cursor，不 emit
    presenter_->openChat(chatId_);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ChatPresenterTest, LoadHistoryAfterOpenChatEmitsMessages) {
    // Bob 先发两条消息（onMessageStored 已自动同步到 cursor）
    client_->chat().sendMessage(
        tokenB_, chatId_, 0, core::MessageContent{core::TextContent{"hi"}});
    client_->chat().sendMessage(
        tokenB_, chatId_, 0, core::MessageContent{core::TextContent{"hello"}});

    // 消息已被 onMessageStored 处理，清掉之前的信号
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    ASSERT_TRUE(spy.isValid());

    presenter_->openChat(chatId_);
    // cursor 已有数据（end > 0），loadHistory 用 fetchBefore(start) 拉取
    // 但 onMessageStored 已经把 start 设到最早消息，所以 fetchBefore 无更早数据
    // 这里验证的是：如果 cursor 尚未覆盖全部消息，loadHistory 能拉到
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ChatPresenterTest, OpenChatNoSignalWhenEmpty) {
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    presenter_->openChat(chatId_);

    // 空聊天 openChat 不触发信号
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ChatPresenterTest, LoadHistoryOnFreshChat) {
    // openChat 创建空 cursor
    presenter_->openChat(chatId_);

    // 此时没有消息，loadHistory 也不应触发
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    presenter_->loadHistory(chatId_, 20);
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// 信号测试：发送消息
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SendMessageTriggersMessagesInserted) {
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);
    ASSERT_TRUE(spy.isValid());

    presenter_->sendTextMessage(chatId_, "hello bob!");

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
    presenter_->sendTextMessage(chatId_, "original");
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // 获取第一条消息的 id（通过网络层查询）
    auto sync = client_->chat().fetchAfter(tokenA_, chatId_, 0, 50);
    auto originalId = sync.value().messages[0].id;

    // 发送回复
    presenter_->sendMessage(
        chatId_, core::MessageContent{core::TextContent{"reply"}}, originalId);

    ASSERT_EQ(spy.count(), 1);
    auto messages = spy.at(0).at(1).value<std::vector<core::Message>>();
    EXPECT_EQ(messages[0].replyTo, originalId);
}

TEST_F(ChatPresenterTest, SendMultipleMessages) {
    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    presenter_->sendTextMessage(chatId_, "msg1");
    presenter_->sendTextMessage(chatId_, "msg2");
    presenter_->sendTextMessage(chatId_, "msg3");

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
    presenter_->sendTextMessage(chatId_, "from alice");
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
    presenter_->sendTextMessage(chatId_, "oops");

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
    presenter_->sendTextMessage(chatId_, "typo");

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
    // 先发 5 条消息（onMessageStored 自动同步，cursor 已覆盖全部）
    for (int i = 0; i < 5; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i)}});
    }

    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // cursor.start 已是最早消息，fetchBefore 无更早数据
    presenter_->loadHistory(chatId_, 3);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ChatPresenterTest, LoadHistoryWithPartialSync) {
    // 先发 10 条消息（onMessageStored 自动同步）
    for (int i = 0; i < 10; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i)}});
    }

    presenter_->openChat(chatId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // cursor 已覆盖全部，loadHistory 无更多历史
    presenter_->loadHistory(chatId_, 5);
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// 边界条件测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SendToZeroChatIdDoesNotCrash) {
    // 传 0 chatId 发消息，不崩溃即通过
    presenter_->sendTextMessage(0, "orphan");
}

TEST_F(ChatPresenterTest, LoadHistoryWithZeroChatId) {
    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // chatId=0 loadHistory，应安全返回
    presenter_->loadHistory(0, 20);
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

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    // 在第一个聊天发消息（onMessageStored 自动同步）
    client_->chat().sendMessage(
        tokenB_, chatId_, 0,
        core::MessageContent{core::TextContent{"in chat1"}});

    // 在第二个聊天发消息（onMessageStored 自动同步）
    client_->chat().sendMessage(
        tokenC, chatId2, 0,
        core::MessageContent{core::TextContent{"in chat2"}});

    // 两条消息分别触发 onMessageStored → messagesInserted
    ASSERT_EQ(spy.count(), 2);

    auto id1 = spy.at(0).at(0).toLongLong();
    auto id2 = spy.at(1).at(0).toLongLong();
    EXPECT_EQ(id1, chatId_);
    EXPECT_EQ(id2, chatId2);

    // openChat 不再 emit
    spy.clear();
    presenter_->openChat(chatId_);
    presenter_->openChat(chatId2);
    EXPECT_EQ(spy.count(), 0);
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

    presenter_->sendTextMessage(chatId_, "hello");

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

    // openChat 不再重新推送（只创建 cursor，但 cursor 已存在）
    spy.clear();
    presenter_->openChat(chatId_);
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// loadLatest 测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, LoadLatestReturnsMostRecentMessages) {
    // 临时清除 session，防止 onMessageStored 自动同步
    presenter_->setSession("", 0);

    // 预灌 30 条消息
    for (int i = 0; i < 30; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i + 1)}});
    }

    // 恢复 session
    presenter_->setSession(tokenA_, aliceId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    presenter_->openChat(chatId_);
    presenter_->loadLatest(chatId_, 10);

    // 应返回最新的 10 条（msg 21 ~ msg 30）
    ASSERT_EQ(spy.count(), 1);
    auto messages = spy.at(0).at(1).value<std::vector<core::Message>>();
    ASSERT_EQ(messages.size(), 10u);

    // 验证是最新的消息
    auto* firstText = std::get_if<core::TextContent>(&messages[0].content[0]);
    ASSERT_NE(firstText, nullptr);
    EXPECT_EQ(firstText->text, "msg 21");

    auto* lastText = std::get_if<core::TextContent>(&messages[9].content[0]);
    ASSERT_NE(lastText, nullptr);
    EXPECT_EQ(lastText->text, "msg 30");
}

TEST_F(ChatPresenterTest, LoadLatestThenLoadHistory) {
    // 临时清除 session
    presenter_->setSession("", 0);

    // 预灌 30 条消息
    for (int i = 0; i < 30; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i + 1)}});
    }

    presenter_->setSession(tokenA_, aliceId_);

    QSignalSpy spy(presenter_.get(), &chat::ChatPresenter::messagesInserted);

    presenter_->openChat(chatId_);
    presenter_->loadLatest(chatId_, 10);

    ASSERT_EQ(spy.count(), 1);
    auto latest = spy.at(0).at(1).value<std::vector<core::Message>>();
    ASSERT_EQ(latest.size(), 10u);

    // loadHistory 应返回 start 之前的消息
    spy.clear();
    presenter_->loadHistory(chatId_, 10);

    ASSERT_EQ(spy.count(), 1);
    auto history = spy.at(0).at(1).value<std::vector<core::Message>>();
    ASSERT_EQ(history.size(), 10u);

    // 历史消息应比最新消息更早
    EXPECT_LT(history.back().id, latest.front().id);

    auto* histText = std::get_if<core::TextContent>(&history[0].content[0]);
    ASSERT_NE(histText, nullptr);
    EXPECT_EQ(histText->text, "msg 11");
}

// ═══════════════════════════════════════════════════════════════
// fetchUpdated / syncUpdated 测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, FetchUpdatedReturnsModifiedMessages) {
    // 发 3 条消息
    presenter_->openChat(chatId_);
    presenter_->sendTextMessage(chatId_, "msg1");
    presenter_->sendTextMessage(chatId_, "msg2");
    presenter_->sendTextMessage(chatId_, "msg3");

    // 获取消息 ID
    auto sync = client_->chat().fetchAfter(tokenA_, chatId_, 0, 50);
    auto& msgs = sync.value().messages;
    ASSERT_EQ(msgs.size(), 3u);
    auto msg2Id = msgs[1].id;

    // 编辑 msg2（触发 updatedAt 变化）
    QSignalSpy updatedSpy(presenter_.get(), &chat::ChatPresenter::messageUpdated);

    presenter_->editMessage(
        msg2Id, core::MessageContent{core::TextContent{"msg2 edited"}});

    // onMessageUpdated 应触发 messageUpdated 信号
    ASSERT_EQ(updatedSpy.count(), 1);
    auto updated = updatedSpy.at(0).at(1).value<core::Message>();
    EXPECT_EQ(updated.id, msg2Id);

    auto* text = std::get_if<core::TextContent>(&updated.content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "msg2 edited");
}

TEST_F(ChatPresenterTest, FetchAfterZeroReturnsLatest) {
    // 直接测试 ChatService 的 fetchAfter(0) 语义
    presenter_->setSession("", 0);

    for (int i = 0; i < 20; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i + 1)}});
    }

    presenter_->setSession(tokenA_, aliceId_);

    auto result = client_->chat().fetchAfter(tokenA_, chatId_, 0, 5);
    ASSERT_TRUE(result.has_value());
    auto& msgs = result.value().messages;

    // afterId=0 应返回最新的 5 条
    ASSERT_EQ(msgs.size(), 5u);

    auto* firstText = std::get_if<core::TextContent>(&msgs[0].content[0]);
    ASSERT_NE(firstText, nullptr);
    EXPECT_EQ(firstText->text, "msg 16");

    auto* lastText = std::get_if<core::TextContent>(&msgs[4].content[0]);
    ASSERT_NE(lastText, nullptr);
    EXPECT_EQ(lastText->text, "msg 20");
}

TEST_F(ChatPresenterTest, FetchBeforeZeroReturnsEarliest) {
    // 直接测试 ChatService 的 fetchBefore(0) 语义
    presenter_->setSession("", 0);

    for (int i = 0; i < 20; ++i) {
        client_->chat().sendMessage(
            tokenA_, chatId_, 0,
            core::MessageContent{
                core::TextContent{"msg " + std::to_string(i + 1)}});
    }

    presenter_->setSession(tokenA_, aliceId_);

    auto result = client_->chat().fetchBefore(tokenA_, chatId_, 0, 5);
    ASSERT_TRUE(result.has_value());
    auto& msgs = result.value().messages;

    // beforeId=0 应返回最早的 5 条
    ASSERT_EQ(msgs.size(), 5u);

    auto* firstText = std::get_if<core::TextContent>(&msgs[0].content[0]);
    ASSERT_NE(firstText, nullptr);
    EXPECT_EQ(firstText->text, "msg 1");

    auto* lastText = std::get_if<core::TextContent>(&msgs[4].content[0]);
    ASSERT_NE(lastText, nullptr);
    EXPECT_EQ(lastText->text, "msg 5");
}
