#include "MockAutoResponder.h"

namespace wechat::chat {

MockAutoResponder::MockAutoResponder(network::NetworkClient& client,
                                     QObject* parent)
    : QObject(parent), client_(client) {}

void MockAutoResponder::setResponderSession(std::string const& token,
                                            std::string const& userId) {
    token_ = token;
    userId_ = userId;
}

void MockAutoResponder::setChatId(std::string const& chatId) {
    chatId_ = chatId;
}

void MockAutoResponder::sendMessage(std::string const& text) {
    core::TextContent tc;
    tc.text = text;
    client_.chat().sendMessage(token_, chatId_, 0, {tc});
}

} // namespace wechat::chat
