#pragma once

#include <wechat/chat/SessionPresenter.h>

#include <QWidget>
#include <QListWidget>

#include <map>
#include <vector>

namespace wechat {
namespace chat {

class SessionListWidget : public QWidget {
    Q_OBJECT

public:
    explicit SessionListWidget(QWidget* parent = nullptr);

    void setPresenter(SessionPresenter* presenter);

Q_SIGNALS:
    void sessionSelected(int64_t chatId);

private Q_SLOTS:
    void onSessionsLoaded(std::vector<SessionItem> sessions);
    void onSessionUpdated(SessionItem session);
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupConnections();
    void updateItem(QListWidgetItem* item, const SessionItem& session);

    QListWidget* listWidget = nullptr;
    SessionPresenter* presenter = nullptr;

    // chatId → list row index
    std::map<int64_t, int> chatIdToRow;
    std::vector<SessionItem> sessions;
};

} // namespace chat
} // namespace wechat
