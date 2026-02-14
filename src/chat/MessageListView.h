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

namespace wechat {
namespace chat {

/**
 * @brief 消息项控件 - 用于在列表中显示单个消息
 */
class MessageItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit MessageItemWidget(QWidget *parent = nullptr);

    void setMessageData(const core::Message& message, const core::User& currentUser);

    // 提供对消息数据的公共访问方法
    const core::Message& getMessage() const { return message_; }

    // 返回推荐的大小
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    core::Message message_;
    core::User currentUser_;
};

/**
 * @brief 自定义 ListView - 消息列表
 */
class MessageListView : public QListWidget {
    Q_OBJECT

public:
    explicit MessageListView(QWidget *parent = nullptr);

    void addMessage(const core::Message& message, const core::User& currentUser);

private slots:
    void onItemClicked(QListWidgetItem *item);

signals:
    void messageSelected(const core::Message& message);
};

} // namespace chat
} // namespace wechat