#include "SessionListWidget.h"

#include <QVBoxLayout>

namespace wechat {
namespace chat {

SessionListWidget::SessionListWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void SessionListWidget::setPresenter(SessionPresenter* presenter) {
    this->presenter = presenter;
    setupConnections();
    presenter->loadSessions();
}

void SessionListWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    listWidget = new QListWidget;
    listWidget->setStyleSheet(
        "QListWidget { background: #2E2E2E; color: white; border: none; "
        "font-size: 14px; }"
        "QListWidget::item { padding: 12px 10px; }"
        "QListWidget::item:selected { background: #3A3A3A; }");
    layout->addWidget(listWidget);
}

void SessionListWidget::setupConnections() {
    connect(listWidget, &QListWidget::itemClicked,
            this, &SessionListWidget::onItemClicked);

    if (presenter) {
        connect(presenter, &SessionPresenter::sessionsLoaded,
                this, &SessionListWidget::onSessionsLoaded);
        connect(presenter, &SessionPresenter::sessionUpdated,
                this, &SessionListWidget::onSessionUpdated);
    }
}

void SessionListWidget::onSessionsLoaded(std::vector<SessionItem> sessions) {
    this->sessions = sessions;
    listWidget->clear();
    chatIdToRow.clear();

    for (int i = 0; i < static_cast<int>(sessions.size()); ++i) {
        auto* item = new QListWidgetItem;
        updateItem(item, sessions[i]);
        item->setData(Qt::UserRole, static_cast<qlonglong>(sessions[i].chatId));
        listWidget->addItem(item);
        chatIdToRow[sessions[i].chatId] = i;
    }
}

void SessionListWidget::onSessionUpdated(SessionItem session) {
    auto it = chatIdToRow.find(session.chatId);
    if (it != chatIdToRow.end()) {
        // 更新已有条目
        auto row = it->second;
        if (row < static_cast<int>(sessions.size())) {
            sessions[row] = session;
            updateItem(listWidget->item(row), session);
        }
    } else {
        // 新会话，添加到顶部
        sessions.insert(sessions.begin(), session);
        auto* item = new QListWidgetItem;
        updateItem(item, session);
        item->setData(Qt::UserRole, static_cast<qlonglong>(session.chatId));
        listWidget->insertItem(0, item);

        // 重建 chatIdToRow
        chatIdToRow.clear();
        for (int i = 0; i < static_cast<int>(sessions.size()); ++i) {
            chatIdToRow[sessions[i].chatId] = i;
        }
    }
}

void SessionListWidget::onItemClicked(QListWidgetItem* item) {
    auto chatId = item->data(Qt::UserRole).toLongLong();
    Q_EMIT sessionSelected(chatId);
}

void SessionListWidget::updateItem(QListWidgetItem* item,
                                   const SessionItem& session) {
    QString text = QString::fromStdString(session.displayName);
    if (!session.lastMessage.empty()) {
        text += "\n" + QString::fromStdString(session.lastMessage);
    }
    item->setText(text);
}

} // namespace chat
} // namespace wechat
