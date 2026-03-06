#include <QApplication>
#include <QDebug>
#include <QListWidget>
#include <QMap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

////////////////////////////////////////////////////////////
// 1. Data Model
////////////////////////////////////////////////////////////

struct Message
{
    int chatSeq;
    qint64 version;
    QString content;
};

// 管理加载过的区间，自动合并重叠部分
class RangeSet
{
public:
    struct Range
    {
        int start;
        int end;
        bool operator<(const Range &other) const { return start < other.start; }
    };

    void addRange(int start, int end)
    {
        ranges.push_back({start, end});
        std::sort(ranges.begin(), ranges.end());

        // 合并相邻或重叠区间
        QVector<Range> merged;
        for (const auto &r : ranges) {
            if (merged.isEmpty() || r.start > merged.last().end + 1) {
                merged.push_back(r);
            } else {
                merged.last().end = std::max(merged.last().end, r.end);
            }
        }
        ranges = merged;
    }

    bool contains(int seq) const
    {
        for (const auto &r : ranges) {
            if (seq >= r.start && seq <= r.end)
                return true;
        }
        return false;
    }

    QVector<Range> ranges;
};

////////////////////////////////////////////////////////////
// 2. MessageStorage (Data Layer)
////////////////////////////////////////////////////////////
class MessageStorage
{
public:
    void upsertMessages(const QList<Message> &msgs)
    {
        for (const auto &m : msgs) {
            cache[m.chatSeq] = m;
            maxVersionSeen = std::max(maxVersionSeen, m.version);
        }
    }
    qint64 getMaxVersion() const { return maxVersionSeen; }
    const Message *get(int seq) const { return cache.contains(seq) ? &cache[seq] : nullptr; }

private:
    QMap<int, Message> cache;
    qint64 maxVersionSeen = 0;
};

////////////////////////////////////////////////////////////
// 3. Fake Server (Network Layer)
////////////////////////////////////////////////////////////

class FakeServer
{
public:
    static constexpr int MaxSeq = 10000;
    static qint64 globalVersion;
    static QMap<int, Message> database;

    static void init()
    {
        for (int i = 1; i <= MaxSeq; ++i) {
            database[i] = {i, ++globalVersion, QString("Message %1").arg(i)};
        }
    }

    static QList<Message> fetch(int seq, int count, bool older)
    {
        QList<Message> res;
        if (older) {
            for (int i = seq - 1; i >= std::max(1, seq - count); --i)
                res.append(database[i]);
        } else {
            for (int i = seq + 1; i <= std::min(MaxSeq, seq + count); ++i)
                res.append(database[i]);
        }
        return res;
    }

    static QList<Message> fetchAround(int seq, int radius)
    {
        QList<Message> res;
        for (int i = std::max(1, seq - radius); i <= std::min(MaxSeq, seq + radius); ++i)
            res.append(database[i]);
        return res;
    }
};
qint64 FakeServer::globalVersion = 0;
QMap<int, Message> FakeServer::database;

////////////////////////////////////////////////////////////
// 4. LazyLoader (Control Layer)
////////////////////////////////////////////////////////////
class LazyLoader : public QObject
{
    Q_OBJECT
public:
    LazyLoader(MessageStorage *s)
        : storage(s)
    {}

    // 加载更旧的消息 (向上滚)
    void loadOlder(int firstSeq)
    {
        if (m_isLoading || firstSeq <= 1)
            return;
        m_isLoading = true;
        QTimer::singleShot(100, this, [=]() {
            auto data = FakeServer::fetch(firstSeq, 20, true);
            storage->upsertMessages(data);
            m_isLoading = false;
            emit dataLoaded(data, "top");
        });
    }

    // 加载更新的消息 (向下滚) - 修复重点
    void loadNewer(int lastSeq)
    {
        if (m_isLoading || lastSeq >= FakeServer::MaxSeq)
            return;
        m_isLoading = true;
        QTimer::singleShot(100, this, [=]() {
            auto data = FakeServer::fetch(lastSeq, 20, false);
            storage->upsertMessages(data);
            m_isLoading = false;
            emit dataLoaded(data, "bottom_expand");
        });
    }

    void jumpTo(int seq)
    {
        m_isLoading = true;
        QTimer::singleShot(100, this, [=]() {
            auto data = FakeServer::fetchAround(seq, 20);
            storage->upsertMessages(data);
            m_isLoading = false;
            emit dataLoaded(data, "jump", seq);
        });
    }

signals:
    void dataLoaded(QList<Message> msgs, QString type, int target = -1);

private:
    MessageStorage *storage;
    bool m_isLoading = false;
};

////////////////////////////////////////////////////////////
// 5. ChatView (UI Layer)
////////////////////////////////////////////////////////////
class ChatWidget : public QWidget
{
    Q_OBJECT
public:
    ChatWidget()
    {
        auto layout = new QVBoxLayout(this);
        listWidget = new QListWidget(this);
        listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        layout->addWidget(listWidget);

        storage = new MessageStorage();
        loader = new LazyLoader(storage);

        connect(listWidget->verticalScrollBar(),
                &QScrollBar::valueChanged,
                this,
                &ChatWidget::handleScroll);
        connect(loader, &LazyLoader::dataLoaded, this, &ChatWidget::onDataLoaded);

        // 初始加载底部
        loader->jumpTo(FakeServer::MaxSeq);

        auto jumpBtn = new QPushButton("Jump to 5000", this);
        layout->addWidget(jumpBtn);
        connect(jumpBtn, &QPushButton::clicked, [this]() { loader->jumpTo(5000); });
    }

private:
    // 核心修复：双向边界检测
    void handleScroll(int value)
    {
        if (listWidget->count() == 0)
            return;

        QScrollBar *bar = listWidget->verticalScrollBar();
        int threshold = 50; // 像素阈值

        // 触顶：加载旧数据
        if (value <= threshold) {
            int firstSeq = listWidget->item(0)->data(Qt::UserRole).toInt();
            loader->loadOlder(firstSeq);
        }
        // 触底：加载新数据 (修复跳转卡顿的关键)
        else if (value >= bar->maximum() - threshold) {
            int lastSeq = listWidget->item(listWidget->count() - 1)->data(Qt::UserRole).toInt();
            loader->loadNewer(lastSeq);
        }
    }

    void onDataLoaded(QList<Message> msgs, QString type, int target)
    {
        if (msgs.isEmpty() && type != "jump")
            return;

        // 排序确保有序插入
        std::sort(msgs.begin(), msgs.end(), [](const Message &a, const Message &b) {
            return a.chatSeq < b.chatSeq;
        });

        if (type == "top") {
            // 向上扩展：必须保存锚点
            QListWidgetItem *anchorItem = listWidget->item(0);
            int oldY = listWidget->visualItemRect(anchorItem).top();

            for (int i = msgs.size() - 1; i >= 0; --i) {
                listWidget->insertItem(0, createItem(msgs[i]));
            }

            // 修正滚动条位置，防止跳动
            int newY = listWidget->visualItemRect(anchorItem).top();
            listWidget->verticalScrollBar()->setValue(listWidget->verticalScrollBar()->value()
                                                      + (newY - oldY));
        } else if (type == "bottom_expand") {
            // 向下扩展：直接 append，不需要特殊处理锚点
            for (const auto &m : msgs) {
                listWidget->addItem(createItem(m));
            }
        } else if (type == "jump") {
            // 跳转：重置视图
            listWidget->clear();
            for (const auto &m : msgs)
                listWidget->addItem(createItem(m));

            // 定位到目标
            for (int i = 0; i < listWidget->count(); ++i) {
                if (listWidget->item(i)->data(Qt::UserRole).toInt() == target) {
                    listWidget->scrollToItem(listWidget->item(i),
                                             QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
        }
    }

    QListWidgetItem *createItem(const Message &m)
    {
        auto *item = new QListWidgetItem(m.content);
        item->setData(Qt::UserRole, m.chatSeq);
        return item;
    }

    QListWidget *listWidget;
    MessageStorage *storage;
    LazyLoader *loader;
};

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);
    // FakeServer::init();
    // ChatWidget w;
    // w.resize(400, 600);
    // w.show();

    auto w = new QWidget;
    w->setStyleSheet(R"({background-color:white})");
    auto l = new QVBoxLayout(w);
    auto b1 = new QPushButton("b1");
    b1->setStyleSheet("QPushButton {"
                      "border: 1px solid #808080;"
                      "border-radius: 0px;"
                      "}");
    auto b2 = new QPushButton("b2");
    b2->setStyleSheet("QPushButton {"
                      "border: 1px solid #b0b0b0;"
                      "border-radius: 0px;"
                      "background: #f6f6f6;"
                      "padding: 0px;"
                      "}"
                      "QPushButton:hover {"
                      "background: #e8e8e8;"
                      "border: 1px solid #909090;"
                      "}"
                      "QPushButton:pressed {"
                      "background: #dcdcdc;"
                      "}");
    b1->setCheckable(true);
    b2->setCheckable(true);
    b2->setFlat(true);
    l->addWidget(b1);
    l->addWidget(b2);
    w->show();
    return a.exec();
}

#include "main.moc"
