#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QDateTime>
#include <QFrame>
#include <QPainter>
#include <QStyleOption>

#include <wechat/core/Message.h>
#include <wechat/core/User.h>

#include <map>

// Forward declaration
namespace wechat {
namespace chat {
class MessageItemWidget;
}
} // namespace wechat

namespace wechat {
namespace chat {

/**
 * @brief 自定义 ListView - 消息列表
 *
 * addMessage 具有 upsert 语义：
 *   - msg.id 不存在 → 创建新 item 并按 id 排序插入
 *   - msg.id 已存在 → 就地更新内容（撤回/编辑）
 */
class MessageListView : public QListWidget {
    Q_OBJECT

public:
    explicit MessageListView(QWidget *parent = nullptr);

    /// upsert：不存在则创建，已存在则更新。不做任何滚动。
    void addMessage(core::Message const &message,
                    core::User const &currentUser);

    /// 当前滚动条是否在底部附近（用于调用方判断是否需要 scrollToBottom）
    bool isAtBottom() const;

    /// 保存当前第一个可见 item 的锚点（用于加载历史后恢复位置）
    void saveScrollAnchor();

    /// 恢复到 saveScrollAnchor 保存的位置
    void restoreScrollAnchor();

    // 获取当前选中的消息项
    MessageItemWidget *getSelectedItem() const {
        return selectedItem_;
    }

    // 设置选中的消息项
    void setSelectedItem(MessageItemWidget *item);

Q_SIGNALS:
    void messageSelected(core::Message const &message);
    void replyRequested(core::Message const &message);
    void forwardRequested(core::Message const &message);
    void revokeRequested(core::Message const &message);

    /// 用户滚动到顶部，请求加载更早的历史消息
    void reachedTop();

private:
    MessageItemWidget *selectedItem_ = nullptr;

    /// msg.id → QListWidgetItem*（用于 upsert 查找）
    std::map<int64_t, QListWidgetItem*> itemById_;

    /// 插入过程中抑制 reachedTop 信号
    bool inserting_ = false;

    /// saveScrollAnchor 保存的锚点 item
    QListWidgetItem* anchorItem_ = nullptr;
};

} // namespace chat
} // namespace wechat
