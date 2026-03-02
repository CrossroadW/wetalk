#include "IconSidebar.h"

#include <QLabel>
#include <QVBoxLayout>

namespace wechat {
namespace app {

IconSidebar::IconSidebar(QWidget* parent) : QWidget(parent) {
    setFixedWidth(60);
    setStyleSheet(R"(
        IconSidebar {
            background-color: #ededed;
            border-right: 1px solid #d6d6d6;
        }
        QPushButton {
            background-color: transparent;
            border: none;
            color: #181818;
            font-size: 20px;
            padding: 15px 0;
        }
        QPushButton:hover {
            background-color: #d6d6d6;
        }
        QPushButton:checked {
            background-color: #c6c6c6;
        }
    )");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 20, 0, 0);
    layout->setSpacing(0);

    // 头像占位
    auto* avatar = new QLabel("👤");
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("font-size: 32px; padding: 10px; color: #181818;");
    layout->addWidget(avatar);

    layout->addSpacing(20);

    // 聊天按钮
    chatBtn = new QPushButton("💬");
    chatBtn->setCheckable(true);
    chatBtn->setChecked(true);
    layout->addWidget(chatBtn);

    // 通讯录按钮
    contactsBtn = new QPushButton("👥");
    contactsBtn->setCheckable(true);
    layout->addWidget(contactsBtn);

    // 朋友圈按钮（占位）
    auto* momentsBtn = new QPushButton("🌐");
    momentsBtn->setCheckable(true);
    momentsBtn->setEnabled(false);
    layout->addWidget(momentsBtn);

    layout->addStretch();

    // 设置按钮（占位）
    auto* settingsBtn = new QPushButton("⚙️");
    layout->addWidget(settingsBtn);

    // 互斥选择
    connect(chatBtn, &QPushButton::clicked, [this]() {
        chatBtn->setChecked(true);
        contactsBtn->setChecked(false);
        Q_EMIT tabChanged(0);
    });

    connect(contactsBtn, &QPushButton::clicked, [this]() {
        chatBtn->setChecked(false);
        contactsBtn->setChecked(true);
        Q_EMIT tabChanged(1);
    });
}

}  // namespace app
}  // namespace wechat
