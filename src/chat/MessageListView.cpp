#include "MessageListView.h"
#include "MessageItemWidget.h"
#include <QPainter>
#include <QStyleOption>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QScrollBar>
#include <QApplication>
#include <QDateTime>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QMouseEvent>

namespace wechat {
namespace chat {

// MessageListView 实现
MessageListView::MessageListView(QWidget *parent) : QListWidget(parent) {
    setStyleSheet("QListWidget {"
                  "    background-color: white;"
                  "    border: none;"
                  "    outline: none;" // 移除焦点轮廓
                  "}"
                  "QListWidget::item {"
                  "    border: none;"
                  "    padding: 0px;"
                  "    margin: 0px;"
                  "}"
                  "QListWidget::item:focus {"
                  "    border: none;"  // 移除选中项的边框
                  "    outline: none;" // 移除焦点虚线框
                  "}");

    // 设置平滑滚动和滚动条行为
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // 平滑滚动
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);     // 禁用水平滚动条
    setResizeMode(QListWidget::Adjust);                       // 自适应大
    // 禁用默认选择行为，因为我们有自己的选择机制
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus); // 完全禁用焦点

    // 滚动到顶部检测
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int value) {
                if (value == verticalScrollBar()->minimum() && count() > 0) {
                    Q_EMIT reachedTop();
                }
            });
}

void MessageListView::addMessage(core::Message const &message,
                                 core::User const &currentUser) {
    // ── upsert：已存在则更新 ──
    auto it = itemById_.find(message.id);
    if (it != itemById_.end()) {
        auto* widget = qobject_cast<MessageItemWidget*>(
            itemWidget(it->second));
        if (widget) {
            widget->updateMessage(message);
            it->second->setSizeHint(widget->sizeHint());
        }
        return;
    }

    // ── 不存在 → 创建新 item ──
    MessageItemWidget *messageWidget = new MessageItemWidget();
    messageWidget->setMessageData(message, currentUser);

    // 连接点击信号
    connect(messageWidget, &MessageItemWidget::clicked, this,
            [this, messageWidget](core::Message const &msg) {
                if (selectedItem_ && selectedItem_ != messageWidget) {
                    selectedItem_->setSelected(false);
                }
                setSelectedItem(messageWidget);
                Q_EMIT messageSelected(msg);
            });

    // 转发右键菜单信号
    connect(messageWidget, &MessageItemWidget::replyRequested, this,
            &MessageListView::replyRequested);
    connect(messageWidget, &MessageItemWidget::forwardRequested, this,
            &MessageListView::forwardRequested);
    connect(messageWidget, &MessageItemWidget::revokeRequested, this,
            &MessageListView::revokeRequested);

    QListWidgetItem *item = new QListWidgetItem();
    QSize idealSize = messageWidget->sizeHint();
    item->setSizeHint(idealSize);

    // ── 按 id 排序插入 ──
    // 找到第一个 id > message.id 的位置
    int insertRow = count(); // 默认追加到末尾
    for (auto jt = itemById_.upper_bound(message.id); jt != itemById_.end(); ++jt) {
        // 找到该 item 在 list 中的 row
        int row = this->row(jt->second);
        if (row >= 0 && row < insertRow) {
            insertRow = row;
            break;
        }
    }

    insertItem(insertRow, item);
    setItemWidget(item, messageWidget);
    itemById_[message.id] = item;


}

void MessageListView::setSelectedItem(MessageItemWidget *item) {
    if (selectedItem_ && selectedItem_ != item) {
        selectedItem_->setSelected(false);
    }
    selectedItem_ = item;
    if (item) {
        item->setSelected(true);
    }
}

} // namespace chat
} // namespace wechat
