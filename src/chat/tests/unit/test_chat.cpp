#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFuture>
#include <QPromise>
#include <QSignalSpy>

#include <wechat/cache/MessageCache.h>
#include <wechat/chat/ChatPresenter.h>
#include <wechat/core/Message.h>
#include <wechat/core/User.h>
#include <wechat/network/ChatTransport.h>

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
    msg.chatSeq = 3;
    msg.timestamp = 1234567890;

    EXPECT_EQ(msg.id, 1);
    EXPECT_EQ(msg.senderId, 100);
    EXPECT_EQ(msg.chatId, 200);
    EXPECT_EQ(msg.chatSeq, 3);
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
// MockChatTransport
// ═══════════════════════════════════════════════════════════════

class MockChatTransport : public wechat::network::ChatTransport {
    Q_OBJECT

public:
    using SyncResult = wechat::network::Result<wechat::network::SyncResponse>;

    explicit MockChatTransport(QObject* parent = nullptr)
        : ChatTransport(parent) {}

    // ── 配置下一次 fetch 返回的数据 ──────────────────────────────

    void setNextMessages(std::vector<wechat::core::Message> msgs,
                         bool hasMore = false) {
        nextMsgs_  = std::move(msgs);
        nextMore_  = hasMore;
    }

    void setNextError(std::string error) {
        nextError_ = std::move(error);
    }

    // ── ChatTransport 接口 ────────────────────────────────────────

    QFuture<SyncResult> fetchLatest(
        const std::string&, int64_t, int) override { return makeResult(); }

    QFuture<SyncResult> fetchBefore(
        const std::string&, int64_t, int32_t, int) override { return makeResult(); }

    QFuture<SyncResult> fetchAfter(
        const std::string&, int64_t, int32_t, int) override { return makeResult(); }

    QFuture<SyncResult> fetchAround(
        const std::string&, int64_t, int32_t, int) override { return makeResult(); }

    QFuture<SyncResult> fetchUpdated(
        const std::string&, int64_t, int64_t, int) override { return makeResult(); }

    void sendMessage(const std::string&, int64_t, int64_t,
                     const wechat::core::MessageContent&) override {}
    void revokeMessage(const std::string&, int64_t) override {}
    void editMessage(const std::string&, int64_t,
                     const wechat::core::MessageContent&) override {}

    // ── 测试辅助：模拟推送 ────────────────────────────────────────

    void simulatePush(int64_t chatId, wechat::core::Message msg) {
        Q_EMIT messagePushed(chatId, msg);
    }

    void simulateChangePush(int64_t chatId, wechat::core::Message msg) {
        Q_EMIT messageChangePushed(chatId, msg);
    }

private:
    QFuture<SyncResult> makeResult() {
        QPromise<SyncResult> promise;
        promise.start();
        if (!nextError_.empty()) {
            promise.addResult(std::unexpected(std::move(nextError_)));
            nextError_.clear();
        } else {
            wechat::network::SyncResponse resp;
            resp.messages = std::move(nextMsgs_);
            resp.hasMore  = nextMore_;
            nextMsgs_.clear();
            nextMore_ = false;
            promise.addResult(SyncResult{resp});
        }
        promise.finish();
        return promise.future();
    }

    std::vector<wechat::core::Message> nextMsgs_;
    bool nextMore_ = false;
    std::string nextError_;
};

// ═══════════════════════════════════════════════════════════════
// ChatPresenter 测试 Fixture
// ═══════════════════════════════════════════════════════════════

Q_DECLARE_METATYPE(std::vector<wechat::core::Message>)
Q_DECLARE_METATYPE(wechat::core::Message)
Q_DECLARE_METATYPE(wechat::cache::InsertHint)

class ChatPresenterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_chat";
            static char* argv[] = {arg0};
            static QCoreApplication app(argc, argv);
        }
        qRegisterMetaType<std::vector<wechat::core::Message>>();
        qRegisterMetaType<wechat::core::Message>();
        qRegisterMetaType<wechat::cache::InsertHint>();
    }

    void SetUp() override {
        transport_ = new MockChatTransport;
        cache_     = std::make_unique<wechat::cache::MessageCache>(*transport_);
        presenter_ = std::make_unique<wechat::chat::ChatPresenter>(*cache_);
        presenter_->setSession("test-token", kAliceId);
    }

    void TearDown() override {
        presenter_.reset();
        cache_.reset();
        delete transport_;
        transport_ = nullptr;
    }

    // 构造一条测试消息
    wechat::core::Message makeMsg(int32_t seq, int64_t chatId = kChatId) {
        wechat::core::Message m;
        m.id      = seq;
        m.chatId  = chatId;
        m.chatSeq = seq;
        m.senderId = kAliceId;
        m.content = {wechat::core::TextContent{"msg " + std::to_string(seq)}};
        return m;
    }

    static constexpr int64_t kAliceId = 1;
    static constexpr int64_t kChatId  = 100;

    MockChatTransport* transport_ = nullptr;  // owned by Qt parent chain? No — raw ptr, deleted in TearDown
    std::unique_ptr<wechat::cache::MessageCache>   cache_;
    std::unique_ptr<wechat::chat::ChatPresenter>   presenter_;
};

// ═══════════════════════════════════════════════════════════════
// 基本状态测试
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, SetSession) {
    EXPECT_EQ(presenter_->currentUserId(), kAliceId);
}

// ═══════════════════════════════════════════════════════════════
// loadLatest → messagesLoaded(InsertHint::Replace)
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, LoadLatestEmitsReplace) {
    transport_->setNextMessages({makeMsg(1), makeMsg(2), makeMsg(3)});

    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messagesLoaded);
    ASSERT_TRUE(spy.isValid());

    presenter_->loadLatest(kChatId, 20);

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args[0].toLongLong(), kChatId);
    auto hint = args[2].value<wechat::cache::InsertHint>();
    EXPECT_EQ(hint, wechat::cache::InsertHint::Replace);
}

TEST_F(ChatPresenterTest, LoadLatestEmptyDoesNotEmit) {
    // setNextMessages with empty list (default)
    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messagesLoaded);
    presenter_->loadLatest(kChatId, 20);
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// loadOlder → messagesLoaded(InsertHint::Top)
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, LoadOlderEmitsTop) {
    // 先 loadLatest 初始化区间
    transport_->setNextMessages({makeMsg(10), makeMsg(11)});
    presenter_->loadLatest(kChatId, 20);

    // 再 loadOlder
    transport_->setNextMessages({makeMsg(8), makeMsg(9)});
    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messagesLoaded);
    presenter_->loadOlder(kChatId, 20);

    ASSERT_EQ(spy.count(), 1);
    auto hint = spy.takeFirst()[2].value<wechat::cache::InsertHint>();
    EXPECT_EQ(hint, wechat::cache::InsertHint::Top);
}

TEST_F(ChatPresenterTest, LoadOlderWithoutLatestDoesNotEmit) {
    // 未 loadLatest，RangeSet 为空，loadOlder 应不请求
    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messagesLoaded);
    presenter_->loadOlder(kChatId, 20);
    EXPECT_EQ(spy.count(), 0);
}

// ═══════════════════════════════════════════════════════════════
// 推送消息 → messagesLoaded(InsertHint::Bottom)
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, PushedMessageEmitsBottom) {
    // 先初始化
    transport_->setNextMessages({makeMsg(1)});
    presenter_->loadLatest(kChatId, 20);

    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messagesLoaded);
    transport_->simulatePush(kChatId, makeMsg(2));

    ASSERT_EQ(spy.count(), 1);
    auto hint = spy.takeFirst()[2].value<wechat::cache::InsertHint>();
    EXPECT_EQ(hint, wechat::cache::InsertHint::Bottom);
}

// ═══════════════════════════════════════════════════════════════
// 推送变更 → messageChanged
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, ChangePushEmitsMessageChanged) {
    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messageChanged);
    ASSERT_TRUE(spy.isValid());

    auto msg = makeMsg(5);
    msg.revoked = true;
    transport_->simulateChangePush(kChatId, msg);

    ASSERT_EQ(spy.count(), 1);
    auto changed = spy.takeFirst()[1].value<wechat::core::Message>();
    EXPECT_EQ(changed.id, 5);
    EXPECT_TRUE(changed.revoked);
}

// ═══════════════════════════════════════════════════════════════
// 网络错误 → loadFailed
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, NetworkErrorEmitsLoadFailed) {
    transport_->setNextError("connection lost");
    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::loadFailed);
    presenter_->loadLatest(kChatId, 20);
    ASSERT_EQ(spy.count(), 1);
}

// ═══════════════════════════════════════════════════════════════
// jumpTo → messagesLoaded(InsertHint::Replace, jumpTarget)
// ═══════════════════════════════════════════════════════════════

TEST_F(ChatPresenterTest, JumpToEmitsReplaceWithTarget) {
    transport_->setNextMessages({makeMsg(48), makeMsg(50), makeMsg(52)});

    QSignalSpy spy(presenter_.get(), &wechat::chat::ChatPresenter::messagesLoaded);
    presenter_->jumpTo(kChatId, 50, 5);

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args[2].value<wechat::cache::InsertHint>(), wechat::cache::InsertHint::Replace);
    EXPECT_EQ(args[3].toInt(), 50);
}

#include "test_chat.moc"
