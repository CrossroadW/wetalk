#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include <wechat/core/Message.h>

#include "MessageListView.h"

#include <string>
#include <vector>

namespace wechat {
namespace chat {

class ChatPresenter;

/**
 * @brief MVP View：主聊天界面
 *
 * 纯展示层，不做任何数据操作。
 * 通过 ChatPresenter 的 3 个信号保持 UI 与数据模型一致。
 *
 * 首次显示时通过 initChat() 调用 openChat + loadLatest 拉取最新数据。
 * 滚动到顶部时自动触发 loadHistory 加载更早的历史消息。
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
    void onMessagesInserted(int64_t chatId,
                            std::vector<core::Message> messages);
    void onMessageUpdated(int64_t chatId, core::Message message);
    void onMessageRemoved(int64_t chatId, int64_t messageId);

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

    /// 首次初始化：openChat + loadLatest（仅执行一次）
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
    bool loading_ = false;
    bool loadingHistory_ = false;  // 区分"加载历史"和"收到新消息"

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
