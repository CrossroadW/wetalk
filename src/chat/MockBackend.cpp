#include "MockBackend.h"

#include <wechat/core/Message.h>

#include <array>
#include <string>

namespace wechat::chat {

static constexpr std::array kMessages = {
    "你好呀 👋",       "今天天气不错",     "在忙什么呢？",
    "哈哈哈 😂",       "好的，收到",       "等一下，我看看",
    "这个问题我想想",   "没问题！",         "晚点再聊",
    "刚吃完饭 🍜",     "周末有空吗？",     "发个红包来 🧧",
    "收到收到 ✅",      "了解了",           "明天见！",
    "这也太搞笑了吧",   "我觉得可以",       "再说吧",
    "好久不见啊",       "最近怎么样？",
};

MockBackend::MockBackend(network::NetworkClient& client, QObject* parent)
    : QObject(parent), client_(client) {
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &MockBackend::executeNext);
}

void MockBackend::setPeerSession(std::string const& token,
                                  int64_t userId) {
    peerToken_ = token;
    peerId_ = userId;
}

void MockBackend::setChatId(int64_t chatId) {
    chatId_ = chatId;
}

// ── 预灌数据 ──

std::vector<int64_t> MockBackend::prefill(
    int count, std::vector<std::string> const& senderTokens) {
    std::vector<int64_t> ids;
    ids.reserve(count);
    for (int i = 0; i < count; ++i) {
        auto const& token = senderTokens[i % senderTokens.size()];
        std::string text =
            "[" + std::to_string(i + 1) + "] " +
            kMessages[i % kMessages.size()];
        core::TextContent tc;
        tc.text = text;
        auto result =
            client_.chat().sendMessage(token, chatId_, 0, {tc});
        if (result.has_value()) {
            ids.push_back(result.value().id);
        }
    }
    return ids;
}

// ── 定时脚本 ──

void MockBackend::runScript(std::vector<ScriptEntry> script) {
    script_ = std::move(script);
    scriptIndex_ = 0;
    sentMsgIds_.clear();

    if (!script_.empty()) {
        timer_.start(script_[0].delayMs);
    }
}

void MockBackend::stop() {
    timer_.stop();
}

int64_t MockBackend::resolveTargetId(int64_t targetMsgId) const {
    if (targetMsgId > 0) {
        return targetMsgId;
    }
    // 负数索引：-n → sentMsgIds_[n-1]
    int idx = static_cast<int>(-targetMsgId) - 1;
    if (idx >= 0 && idx < static_cast<int>(sentMsgIds_.size())) {
        return sentMsgIds_[idx];
    }
    return 0;
}

void MockBackend::executeNext() {
    if (scriptIndex_ >= static_cast<int>(script_.size())) {
        Q_EMIT scriptFinished();
        return;
    }

    auto const& entry = script_[scriptIndex_++];

    switch (entry.action) {
    case Action::Send: {
        core::TextContent tc;
        tc.text = entry.text;
        auto result =
            client_.chat().sendMessage(peerToken_, chatId_, 0, {tc});
        if (result.has_value()) {
            sentMsgIds_.push_back(result.value().id);
        }
        break;
    }
    case Action::Revoke: {
        auto id = resolveTargetId(entry.targetMsgId);
        if (id > 0) {
            client_.chat().revokeMessage(peerToken_, id);
        }
        break;
    }
    case Action::Edit: {
        auto id = resolveTargetId(entry.targetMsgId);
        if (id > 0) {
            core::TextContent tc;
            tc.text = entry.text;
            client_.chat().editMessage(peerToken_, id, {tc});
        }
        break;
    }
    }

    // 调度下一步
    if (scriptIndex_ < static_cast<int>(script_.size())) {
        timer_.start(script_[scriptIndex_].delayMs);
    } else {
        Q_EMIT scriptFinished();
    }
}

std::vector<MockBackend::ScriptEntry> MockBackend::typicalScript() {
    return {
        {500,  Action::Send,   "对方消息 1",              0},
        {300,  Action::Send,   "对方消息 2",              0},
        {800,  Action::Send,   "对方消息 3",              0},
        {400,  Action::Send,   "对方消息 4",              0},
        {600,  Action::Send,   "对方消息 5",              0},
        {1000, Action::Revoke, "",                        -2},
        {500,  Action::Edit,   "对方消息 4（已编辑）",      -4},
        {700,  Action::Send,   "对方消息 6",              0},
        {300,  Action::Send,   "对方消息 7",              0},
        {500,  Action::Send,   "对方消息 8",              0},
    };
}

} // namespace wechat::chat
