#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QDateTime>
#include <QFrame>
#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextDocument>
#include <QMenu>

#include <wechat/core/Message.h>
#include <wechat/core/User.h>

namespace wechat {
namespace chat {

/**
 * @brief 消息项控件 - 用于在列表中显示单个消息
 *
 * 设计要点:
 * 1. 背景显示: 根据发送者身份显示不同背景色
 *    - 自己的消息: rgb(200, 220, 255) - 蓝色背景，右对齐
 *    - 对方消息: rgb(245, 245, 245) - 浅灰色背景，左对齐
 * 2. 交互功能:
 *    - 支持文本选择但禁用键盘编辑（避免插入光标）
 *    - 图片保持比例缩放，最大250x200，点击可预览
 *    - 支持右键菜单（复制、转发、回复）
 *    - 点击选中消息项并触发选中边框
 * 3. 布局规范:
 *    - 气泡圆角: 8px
 *    - 气泡内边距: 8px
 *    - 时间标签: 灰色，右对齐
 * 4. 内容处理:
 *    - 文本内容自动换行
 *    - 资源内容显示相应图标/描述
 */
class MessageItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit MessageItemWidget(QWidget *parent = nullptr);

    void setMessageData(core::Message const &message,
                        core::User const &currentUser);

    core::Message const &getMessage() const {
        return message_;
    }

    // 检查点击位置是否在有效内容区域内
    bool isClickOnContent(QPoint const &pos) const;

    // 设置选中状态
    void setSelected(bool selected);

    bool isSelected() const {
        return selected_;
    }

    // 更新整个消息显示
    void updateMessageDisplay();
    void updateTextLabelsMaxWidth(int maxWidth);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // ⭐ 新增
    bool eventFilter(QObject *watched, QEvent *event)override;

private:
    core::Message message_;
    core::User currentUser_;
    bool selected_ = false;
    QString baseBubbleStyle;                 // 当前气泡基础样式
    QMap<QLabel *, QPixmap> originalPixmaps; // 保存原始图片
    // 用于布局的控件
    QVBoxLayout *mainLayout;
    QHBoxLayout *contentLayout;
    QLabel *timeLabel;
    QWidget *bubbleWidget;     // 消息气泡容器
    QVBoxLayout *bubbleLayout; // 气泡内容布局

    // 用于处理消息内容的控件
    std::vector<QWidget *> contentWidgets;

    // 用于处理右键菜单
    void showContextMenu(QPoint const &pos);

signals:
    void clicked(core::Message const &message);
};

} // namespace chat
} // namespace wechat
