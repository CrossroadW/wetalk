#include <wechat/chat/ChatPresenter.h>

namespace wechat::chat {

ChatPresenter::ChatPresenter(cache::MessageCache& cache, QObject* parent)
    : QObject(parent), cache_(cache)
{
    connect(&cache_, &cache::MessageCache::messagesLoaded,
            this,    &ChatPresenter::messagesLoaded);
    connect(&cache_, &cache::MessageCache::messageChanged,
            this,    &ChatPresenter::messageChanged);
    connect(&cache_, &cache::MessageCache::loadFailed,
            this,    &ChatPresenter::loadFailed);
}

ChatPresenter::~ChatPresenter() = default;

void ChatPresenter::setSession(std::string const& token, int64_t userId) {
    userId_ = userId;
    cache_.setSession(token, userId);
}

int64_t ChatPresenter::currentUserId() const { return userId_; }

// ── 加载操作 ────────────────────────────────────────────────────────────────

void ChatPresenter::loadLatest(int64_t chatId, int limit) {
    cache_.loadLatest(chatId, limit);
}

void ChatPresenter::loadOlder(int64_t chatId, int limit) {
    cache_.loadOlder(chatId, limit);
}

void ChatPresenter::loadNewer(int64_t chatId, int limit) {
    cache_.loadNewer(chatId, limit);
}

void ChatPresenter::jumpTo(int64_t chatId, int32_t chatSeq, int radius) {
    cache_.jumpTo(chatId, chatSeq, radius);
}

// ── 写操作 ───────────────────────────────────────────────────────────────────

void ChatPresenter::sendTextMessage(int64_t chatId, std::string const& text) {
    core::TextContent tc;
    tc.text = text;
    sendMessage(chatId, {tc});
}

void ChatPresenter::sendMessage(int64_t chatId,
                                 core::MessageContent const& content,
                                 int64_t replyTo) {
    cache_.sendMessage(chatId, content, replyTo);
}

void ChatPresenter::revokeMessage(int64_t msgId) {
    cache_.revokeMessage(msgId);
}

void ChatPresenter::editMessage(int64_t msgId,
                                 core::MessageContent const& content) {
    cache_.editMessage(msgId, content);
}

} // namespace wechat::chat
