#include "MockAutoResponder.h"

#include <wechat/core/Message.h>

namespace wechat::chat {

MockAutoResponder::MockAutoResponder(network::NetworkClient& client,
                                     QObject* parent)
    : QObject(parent), client_(client) {
    conn_ = client_.chat().onMessageStored.connect(
        [this](const std::string& chatId) {
            onMessageStored(chatId);
        });
}

void MockAutoResponder::setResponderSession(std::string const& token,
                                            std::string const& userId) {
    token_ = token;
    userId_ = userId;
}

void MockAutoResponder::setChatId(std::string const& chatId) {
    chatId_ = chatId;
}

void MockAutoResponder::onMessageStored(std::string const& chatId) {
    // 只关心自己负责的聊天
    if (chatId != chatId_ || token_.empty()) {
        return;
    }

    // 防止自己的回复再次触发回调（递归保护）
    if (responding_) {
        return;
    }

    // 拉取 lastSeenId_ 之后的新消息
    auto result = client_.chat().fetchAfter(token_, chatId_, lastSeenId_, 50);
    if (!result.ok() || result.value().messages.empty()) {
        return;
    }

    auto& msgs = result.value().messages;
    lastSeenId_ = msgs.back().id;

    // 对每条非自己发的消息进行 echo 回复
    for (auto const& msg : msgs) {
        if (msg.senderId == userId_) {
            continue;
        }

        // 提取文本内容
        std::string text;
        for (auto const& block : msg.content) {
            if (auto* tc = std::get_if<core::TextContent>(&block)) {
                text = tc->text;
                break;
            }
        }
        if (text.empty()) {
            text = "[echo]";
        }

        responding_ = true;
        core::TextContent tc;
        tc.text = "echo: " + text;
        client_.chat().sendMessage(token_, chatId_, 0, {tc});
        responding_ = false;
    }
}

} // namespace wechat::chat
