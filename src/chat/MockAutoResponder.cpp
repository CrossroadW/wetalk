#include "MockAutoResponder.h"

#include <wechat/core/Message.h>

#include <QRandomGenerator>

#include <array>
#include <string>

namespace wechat::chat {

// 预定义的消息模板
static constexpr std::array kMessages = {
    "你好呀 👋",
    "今天天气不错",
    "在忙什么呢？",
    "哈哈哈 😂",
    "好的，收到",
    "等一下，我看看",
    "这个问题我想想",
    "没问题！",
    "晚点再聊",
    "刚吃完饭 🍜",
    "周末有空吗？",
    "发个红包来 🧧",
    "收到收到 ✅",
    "了解了",
    "明天见！",
    "这也太搞笑了吧",
    "我觉得可以",
    "再说吧",
    "好久不见啊",
    "最近怎么样？",
};

MockAutoResponder::MockAutoResponder(network::NetworkClient& client,
                                     QObject* parent)
    : QObject(parent), client_(client) {
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &MockAutoResponder::sendOne);
}

void MockAutoResponder::setResponderSession(std::string const& token,
                                            std::string const& userId) {
    token_ = token;
    userId_ = userId;
}

void MockAutoResponder::setChatId(std::string const& chatId) {
    chatId_ = chatId;
    sentCount_ = 0;
    scheduleNext();
}

void MockAutoResponder::stop() {
    timer_.stop();
}

void MockAutoResponder::scheduleNext() {
    if (sentCount_ >= maxMessages_ || token_.empty() || chatId_.empty()) {
        return;
    }
    // 随机延时 200~2000ms
    int delay = QRandomGenerator::global()->bounded(200, 2001);
    timer_.start(delay);
}

void MockAutoResponder::sendOne() {
    if (sentCount_ >= maxMessages_ || token_.empty() || chatId_.empty()) {
        return;
    }

    // 从模板中随机选一条，加上序号
    int idx = QRandomGenerator::global()->bounded(
        static_cast<int>(kMessages.size()));
    std::string text = "[" + std::to_string(sentCount_ + 1) + "] "
                       + kMessages[idx];

    core::TextContent tc;
    tc.text = text;
    client_.chat().sendMessage(token_, chatId_, 0, {tc});

    ++sentCount_;
    scheduleNext();
}

} // namespace wechat::chat
