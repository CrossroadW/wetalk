#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include <wechat/cache/InsertHint.h>
#include <wechat/core/Message.h>

#include "MessageListView.h"

#include <vector>

namespace wechat {
namespace chat {

class ChatPresenter;

/**
 * @brief MVP View：主聊天界面
 *
 * 纯展示层，不做任何数据操作。
 * 通过 ChatPresenter 的信号保持 UI 与数据模型一致。
 *
 * 首次显示时通过 initChat() 调用 loadLatest 拉取最新数据。
 * 滚动到顶部时自动触发 loadOlder 加载更早的历史消息。
 */
class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget* parent = nullptr);

    void setCurrentUser(core::User const& user);
    void setChatPartner(core::User const& partner);
    void setChatId(int64_t chatId);

    /// 注入 ChatPresenter，ChatWidget 不拥有其生命周期
    void setPresenter(ChatPresenter* presenter);

    MessageListView* getMessageListView() { return messageListView_; }

    void sendMessage();

private Q_SLOTS:
    // 模型变化回调
    void onMessagesLoaded(int64_t chatId,
                          std::vector<core::Message> messages,
                          wechat::cache::InsertHint hint,
                          int32_t jumpTarget);
    void onMessageChanged(int64_t chatId, core::Message message);

    // 右键菜单
    void onReplyRequested(core::Message const& message);
    void onForwardRequested(core::Message const& message);
    void onRevokeRequested(core::Message const& message);

    // 取消回复
    void cancelReply();

    // 滚动到顶部 → 加载历史
    void onReachedTop();

private:
    void setupUI();
    void setupConnections();

    /// 首次初始化：loadLatest（仅执行一次）
    void initChat();

    MessageListView* messageListView_;
    QLineEdit* messageInput_;
    QPushButton* sendButton_;
    QLabel* titleLabel_;

    core::User currentUser_;
    core::User chatPartner_;
    int64_t chatId_ = 0;

    ChatPresenter* presenter_ = nullptr;
    bool initialized_ = false;

    // toast 提示
    QLabel* toastLabel_ = nullptr;

    // 回复状态（纯 UI 状态）
    int64_t replyToMessageId_ = 0;
    QWidget* replyIndicator_ = nullptr;
    QLabel* replyLabel_ = nullptr;
    QPushButton* cancelReplyButton_ = nullptr;
};

} // namespace chat
} // namespace wechat
