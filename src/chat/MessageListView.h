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

// Forward declaration
namespace wechat {
namespace chat {
    class MessageItemWidget;
}
}

namespace wechat {
namespace chat {

/**
 * @brief 自定义 ListView - 消息列表
 */
class MessageListView : public QListWidget {
    Q_OBJECT

public:
    explicit MessageListView(QWidget *parent = nullptr);

    void addMessage(const core::Message& message, const core::User& currentUser);

    // 获取当前选中的消息项
    MessageItemWidget* getSelectedItem() const { return selectedItem_; }

    // 设置选中的消息项
    void setSelectedItem(MessageItemWidget* item);

signals:
    void messageSelected(const core::Message& message);

private:
    MessageItemWidget* selectedItem_ = nullptr;
};

} // namespace chat
} // namespace wechat