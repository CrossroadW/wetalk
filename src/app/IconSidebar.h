#pragma once

#include <QPushButton>
#include <QWidget>

namespace wechat {
namespace app {

/// 左侧图标栏（WeChat 风格）
class IconSidebar : public QWidget {
    Q_OBJECT

public:
    explicit IconSidebar(QWidget* parent = nullptr);

Q_SIGNALS:
    void tabChanged(int index);

private:
    QPushButton* chatBtn;
    QPushButton* contactsBtn;
};

}  // namespace app
}  // namespace wechat
