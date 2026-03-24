#include <wechat/cache/MessageCache.h>

#include "MessageStore.h"
#include "RangeSet.h"

#include <wechat/network/ChatTransport.h>

#include <map>

namespace wechat {
namespace cache {

// ── 每个聊天的状态 ──────────────────────────────────────────────────────────

struct ChatState {
    RangeSet   ranges;
    int64_t    maxUpdatedAt = 0;
    bool       isLoading    = false;
};

// ── Impl ────────────────────────────────────────────────────────────────────

struct MessageCache::Impl {
    network::ChatTransport& transport;
    MessageStore            store;
    std::string             token;
    int64_t                 userId = 0;
    std::map<int64_t, ChatState> chats; // chatId → state

    explicit Impl(network::ChatTransport& t, const std::string& dbPath)
        : transport(t), store(dbPath) {}

    ChatState& state(int64_t chatId) { return chats[chatId]; }
};

// ── 构造/析构 ────────────────────────────────────────────────────────────────

MessageCache::MessageCache(network::ChatTransport& transport,
                           const std::string& dbPath,
                           QObject* parent)
    : QObject(parent)
    , impl_(std::make_unique<Impl>(transport, dbPath))
{
    connect(&transport, &network::ChatTransport::messagePushed,
            this,       &MessageCache::onMessagePushed);
    connect(&transport, &network::ChatTransport::messageChangePushed,
            this,       &MessageCache::onMessageChangePushed);
}

MessageCache::~MessageCache() = default;

// ── 会话 ─────────────────────────────────────────────────────────────────────

void MessageCache::setSession(std::string token, int64_t userId) {
    impl_->token  = std::move(token);
    impl_->userId = userId;
}

// ── loadLatest ───────────────────────────────────────────────────────────────

void MessageCache::loadLatest(int64_t chatId, int limit) {
    auto& st = impl_->state(chatId);
    if (st.isLoading) return;
    st.isLoading = true;

    impl_->transport.fetchLatest(impl_->token, chatId, limit)
        .then(this, [this, chatId](network::Result<network::SyncResponse> result) {
            auto& st = impl_->state(chatId);
            st.isLoading = false;

            if (!result) {
                Q_EMIT loadFailed(chatId, QString::fromStdString(result.error()));
                return;
            }

            const auto& msgs = result->messages;
            if (msgs.empty()) return;

            impl_->store.upsertMessages(msgs);

            // Reset ranges to the newly loaded window
            st.ranges.clear();
            st.ranges.addRange(msgs.front().chatSeq, msgs.back().chatSeq);
            st.maxUpdatedAt = impl_->store.maxUpdatedAt(chatId);

            Q_EMIT messagesLoaded(chatId, msgs, InsertHint::Replace);
        });
}

// ── loadOlder ────────────────────────────────────────────────────────────────

void MessageCache::loadOlder(int64_t chatId, int limit) {
    auto& st = impl_->state(chatId);
    if (st.isLoading || st.ranges.isEmpty()) return;
    st.isLoading = true;

    int32_t firstSeq = st.ranges.firstSeq();

    impl_->transport.fetchBefore(impl_->token, chatId, firstSeq, limit)
        .then(this, [this, chatId](network::Result<network::SyncResponse> result) {
            auto& st = impl_->state(chatId);
            st.isLoading = false;

            if (!result) {
                Q_EMIT loadFailed(chatId, QString::fromStdString(result.error()));
                return;
            }

            const auto& msgs = result->messages;
            if (msgs.empty()) return;

            impl_->store.upsertMessages(msgs);
            st.ranges.addRange(msgs.front().chatSeq, msgs.back().chatSeq);

            Q_EMIT messagesLoaded(chatId, msgs, InsertHint::Top);
        });
}

// ── loadNewer ────────────────────────────────────────────────────────────────

void MessageCache::loadNewer(int64_t chatId, int limit) {
    auto& st = impl_->state(chatId);
    if (st.isLoading || st.ranges.isEmpty()) return;
    st.isLoading = true;

    int32_t lastSeq = st.ranges.lastSeq();

    impl_->transport.fetchAfter(impl_->token, chatId, lastSeq, limit)
        .then(this, [this, chatId](network::Result<network::SyncResponse> result) {
            auto& st = impl_->state(chatId);
            st.isLoading = false;

            if (!result) {
                Q_EMIT loadFailed(chatId, QString::fromStdString(result.error()));
                return;
            }

            const auto& msgs = result->messages;
            if (msgs.empty()) return;

            impl_->store.upsertMessages(msgs);
            st.ranges.addRange(msgs.front().chatSeq, msgs.back().chatSeq);

            Q_EMIT messagesLoaded(chatId, msgs, InsertHint::Bottom);
        });
}

// ── jumpTo ───────────────────────────────────────────────────────────────────

void MessageCache::jumpTo(int64_t chatId, int32_t chatSeq, int radius) {
    auto& st = impl_->state(chatId);
    if (st.isLoading) return;
    st.isLoading = true;

    impl_->transport.fetchAround(impl_->token, chatId, chatSeq, radius)
        .then(this, [this, chatId, chatSeq](network::Result<network::SyncResponse> result) {
            auto& st = impl_->state(chatId);
            st.isLoading = false;

            if (!result) {
                Q_EMIT loadFailed(chatId, QString::fromStdString(result.error()));
                return;
            }

            const auto& msgs = result->messages;
            if (msgs.empty()) return;

            impl_->store.upsertMessages(msgs);

            st.ranges.clear();
            st.ranges.addRange(msgs.front().chatSeq, msgs.back().chatSeq);

            Q_EMIT messagesLoaded(chatId, msgs, InsertHint::Replace, chatSeq);
        });
}

// ── 写操作（fire-and-forget）────────────────────────────────────────────────

void MessageCache::sendMessage(int64_t chatId,
                                const core::MessageContent& content,
                                int64_t replyTo) {
    impl_->transport.sendMessage(impl_->token, chatId, replyTo, content);
}

void MessageCache::revokeMessage(int64_t msgId) {
    impl_->transport.revokeMessage(impl_->token, msgId);
}

void MessageCache::editMessage(int64_t msgId,
                                const core::MessageContent& content) {
    impl_->transport.editMessage(impl_->token, msgId, content);
}

// ── 推送处理 ─────────────────────────────────────────────────────────────────

void MessageCache::onMessagePushed(int64_t chatId, core::Message msg) {
    impl_->store.upsertMessage(msg);

    auto& st = impl_->state(chatId);
    if (!st.ranges.isEmpty()) {
        st.ranges.addRange(msg.chatSeq, msg.chatSeq);
    }

    // Treat a new pushed message as a bottom append
    Q_EMIT messagesLoaded(chatId, {msg}, InsertHint::Bottom);
}

void MessageCache::onMessageChangePushed(int64_t chatId, core::Message msg) {
    impl_->store.upsertMessage(msg);
    Q_EMIT messageChanged(chatId, std::move(msg));
}

} // namespace cache
} // namespace wechat
