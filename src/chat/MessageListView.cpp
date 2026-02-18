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
}

void MessageListView::addMessage(core::Message const &message,
                                 core::User const &currentUser) {
    // 创建自定义的消息项
    MessageItemWidget *messageWidget = new MessageItemWidget();

    // 先设置数据，这样sizeHint可以基于内容计算
    messageWidget->setMessageData(message, currentUser);

    // 连接点击信号
    connect(messageWidget, &MessageItemWidget::clicked, this,
            [this, messageWidget](core::Message const &msg) {
                // 实现单选功能：点击一个消息时，取消之前选中的消息
                if (selectedItem_ && selectedItem_ != messageWidget) {
                    selectedItem_->setSelected(false);
                }

                // 设置新的选中项
                setSelectedItem(messageWidget);

                // 发出消息选中信号
                emit messageSelected(msg);
            });

    // 转发右键菜单信号
    connect(messageWidget, &MessageItemWidget::replyRequested, this,
            &MessageListView::replyRequested);
    connect(messageWidget, &MessageItemWidget::forwardRequested, this,
            &MessageListView::forwardRequested);
    connect(messageWidget, &MessageItemWidget::revokeRequested, this,
            &MessageListView::revokeRequested);

    // 创建列表项
    QListWidgetItem *item = new QListWidgetItem();

    // 获取widget的理想大小并设置给列表项
    QSize idealSize = messageWidget->sizeHint();
    item->setSizeHint(QSize(idealSize.width(), idealSize.height()));

    // 添加到列表
    addItem(item);
    setItemWidget(item, messageWidget);

    // 滚动到底部
    scrollToBottom();
}

void MessageListView::setSelectedItem(MessageItemWidget *item) {
    // 取消之前选中的项
    if (selectedItem_ && selectedItem_ != item) {
        selectedItem_->setSelected(false);
    }

    // 设置新选中的项
    selectedItem_ = item;

    if (item) {
        item->setSelected(true);
    }
}

} // namespace chat
} // namespace wechat
