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

    /// upsert：不存在则创建，已存在则更新
    void addMessage(core::Message const &message,
                    core::User const &currentUser);

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
};

} // namespace chat
} // namespace wechat
