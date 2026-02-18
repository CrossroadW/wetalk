#include "MockAutoResponder.h"

namespace wechat::chat {

MockAutoResponder::MockAutoResponder(network::NetworkClient &client,
                                     QObject *parent)
    : QObject(parent), client_(client) {}

void MockAutoResponder::setResponderSession(std::string const &token,
                                            std::string const &userId) {
    token_ = token;
    userId_ = userId;
}

void MockAutoResponder::setChatId(std::string const &chatId) {
    chatId_ = chatId;
}

void MockAutoResponder::scheduleMessage(std::string const &text, int delayMs) {
    // 捕获 text 副本，延迟后通过 network 层以模拟用户身份发送
    auto textCopy = text;
    QTimer::singleShot(delayMs, this, [this, textCopy]() {
        core::TextContent tc;
        tc.text = textCopy;
        client_.chat().sendMessage(token_, chatId_, "", {tc});
    });
}

void MockAutoResponder::stop() {
    // 停止所有待发的定时器（QObject 析构时也会自动清理子 timer）
}

} // namespace wechat::chat
