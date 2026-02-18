#include <QTimer>
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
#include <QMenu>
#include <QAction>
#include <QMimeData>
#include <QClipboard>
#include <QLabel>
#include <QResizeEvent>

#include "MessageItemWidget.h"
#include "MessageUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFontMetrics>
#include <QListWidget>
#include <QListWidgetItem>

namespace wechat {
namespace chat {

// 图片限制
static int const IMAGE_MAX_WIDTH = 250;
static int const IMAGE_MAX_HEIGHT = 200;

MessageItemWidget::MessageItemWidget(QWidget *parent) : QWidget(parent) {
    mainLayout = new QVBoxLayout(this);
    contentLayout = new QHBoxLayout();
    timeLabel = new QLabel();
    bubbleWidget = new QWidget();
    bubbleLayout = new QVBoxLayout(bubbleWidget);

    // 允许样式表绘制背景（必须）
    bubbleWidget->setAttribute(Qt::WA_StyledBackground, true);

    // 布局与样式基础（减小边距）
    bubbleLayout->setSizeConstraint(QLayout::SetFixedSize);

    bubbleLayout->setContentsMargins(12, 8, 12, 8);
    bubbleLayout->setSpacing(4); // 文本块与图片块之间的间距
    timeLabel->setStyleSheet("color: gray;");
    timeLabel->setAlignment(Qt::AlignRight);

    mainLayout->addLayout(contentLayout);
    mainLayout->addWidget(timeLabel);
    mainLayout->setContentsMargins(10, 2, 10, 2); // 减小垂直间距 (上下仅 2px)
    mainLayout->setSpacing(2);                    // 气泡与时间标签之间的距离
    contentLayout->addWidget(bubbleWidget);
    contentLayout->setAlignment(bubbleWidget, Qt::AlignLeft);

    // 允许在垂直方向上根据内容伸展（解决文本高度被截断的问题）
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    bubbleWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // 默认样式（对方消息浅灰色背景）
    baseBubbleStyle =
        "background-color: rgb(255,255,255); border-radius: 8px; ";
    bubbleWidget->setStyleSheet(baseBubbleStyle);

    // 安装事件过滤器：捕获 bubble 本身的鼠标事件
    bubbleWidget->installEventFilter(this);
    // 注意：我们还会在 updateMessageDisplay 为每个内容控件安装
    // eventFilter(this)
}

void MessageItemWidget::setMessageData(core::Message const &message,
                                       core::User const &currentUser) {
    message_ = message;
    currentUser_ = currentUser;

    bool isSelf = (message_.senderId == currentUser_.id);

    // 只用纯背景色 + 圆角 + padding（不要 border）
    QColor baseColor = isSelf ? QColor(200, 220, 255) : QColor(245, 245, 245);
    baseBubbleStyle = QString("background-color: rgb(%1,%2,%3); border-radius: "
                              "8px; padding: 0px;")
                          .arg(baseColor.red())
                          .arg(baseColor.green())
                          .arg(baseColor.blue());

    // 应用 stylesheet（使样式立即生效）
    bubbleWidget->setStyleSheet(baseBubbleStyle);

    // 也设置 palette + autoFillBackground（提高兼容性）
    bubbleWidget->setAutoFillBackground(true);
    QPalette pal = bubbleWidget->palette();
    pal.setColor(QPalette::Window, baseColor);
    bubbleWidget->setPalette(pal);

    // 左右对齐
    if (isSelf) {
        contentLayout->setAlignment(bubbleWidget, Qt::AlignRight);
    } else {
        contentLayout->setAlignment(bubbleWidget, Qt::AlignLeft);
    }

    // 生成/刷新内容（在这里会为每个生成的内容控件安装 eventFilter）
    updateMessageDisplay();

    // 更新时间标签
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(message_.timestamp);
    timeLabel->setText(dt.toString("hh:mm"));

    // 让布局更新（如果在 QListWidget 中，后续 resizeEvent 也会尝试更新 item
    // 高度）
    // updateGeometry();
}

void MessageItemWidget::updateTextLabelsMaxWidth(int maxWidth) {
    // 防止太小的值导致布局异常
    int w = qMax(40, maxWidth);

    for (QWidget *wgt: contentWidgets) {
        QLabel *lbl = qobject_cast<QLabel *>(wgt);
        if (!lbl) {
            continue;
        }

        // 跳过图片（图片由 originalPixmaps 管理）
        if (originalPixmaps.contains(lbl)) {
            continue;
        }

        // 更新宽度约束与策略，重新换行并调整高度
        lbl->setMaximumWidth(w);
        lbl->setMinimumWidth(0);
        lbl->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        lbl->setWordWrap(true);
        lbl->adjustSize();
    }

    // bubbleLayout->activate();
    // bubbleWidget->updateGeometry();
    // bubbleWidget->adjustSize();
    // updateGeometry();
}

void MessageItemWidget::updateMessageDisplay() {
    // 清理旧内容
    for (auto w: contentWidgets) {
        bubbleLayout->removeWidget(w);
        w->deleteLater();
    }
    contentWidgets.clear();
    originalPixmaps.clear();

    // 估算一个合理的内部宽度用于文本换行：
    // 优先使用当前 bubbleWidget 的宽度（考虑 padding），没有宽度时回退到 300
    int padding = bubbleLayout->contentsMargins().left() +
                  bubbleLayout->contentsMargins().right();
    int estInnerWidth = 300;
    if (bubbleWidget->width() > 0) {
        estInnerWidth = qMax(40, bubbleWidget->width() - padding);
    }

    // 逐块构建（并为每个内容控件安装事件过滤器，以便任意子控件点击都能被检测到）
    for (auto const &block: message_.content) {
        std::visit(
            [this, estInnerWidth](auto const &content_block) {
                using T = std::decay_t<decltype(content_block)>;

                if constexpr (std::is_same_v<T, core::TextContent>) {
                    QLabel *label =
                        new QLabel(QString::fromStdString(content_block.text));
                    label->setWordWrap(true);
                    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

                    // 允许水平扩展并在垂直方向根据需要增长，使用当前估算宽度约束换行
                    label->setSizePolicy(QSizePolicy::Preferred,
                                         QSizePolicy::Preferred);
                    label->setMaximumWidth(estInnerWidth);
                    label->setMinimumWidth(0);

                    bubbleLayout->addWidget(label);
                    contentWidgets.push_back(label);

                    // 安装事件过滤器，使点击文本也能触发选中
                    label->installEventFilter(this);

                    // 不立即固定高度，让布局来计算；仍可调用 adjustSize
                    // 在稍后触发
                } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
                    if (content_block.type == core::ResourceType::Image) {
                        QLabel *imageLabel = new QLabel();
                        imageLabel->setAlignment(Qt::AlignCenter);
                        imageLabel->setCursor(Qt::PointingHandCursor);

                        QString path = getResourcePath(content_block);
                        QPixmap orig(path);
                        if (!orig.isNull()) {
                            // 保存原图，供 resize 时重新按比例缩放
                            originalPixmaps.insert(imageLabel, orig);

                            // 初次缩放到最大限制（使用 estInnerWidth
                            // 使图片在窄气泡下自适应）
                            QPixmap scaled = orig.scaled(
                                estInnerWidth, IMAGE_MAX_HEIGHT,
                                Qt::KeepAspectRatio, Qt::SmoothTransformation);
                            imageLabel->setPixmap(scaled);
                            imageLabel->setFixedSize(scaled.size());
                            imageLabel->setSizePolicy(QSizePolicy::Fixed,
                                                      QSizePolicy::Fixed);
                        } else {
                            imageLabel->setText(
                                QString("[无法加载图片: %1]").arg(path));
                            imageLabel->setSizePolicy(QSizePolicy::Preferred,
                                                      QSizePolicy::Minimum);
                        }

                        bubbleLayout->addWidget(imageLabel);
                        contentWidgets.push_back(imageLabel);

                        // 安装事件过滤器：点击图片也要触发选中
                        imageLabel->installEventFilter(this);
                    } else {
                        QString resourceText = "[文件]";
                        if (content_block.type == core::ResourceType::Video) {
                            resourceText = "[视频]";
                        }
                        if (content_block.type == core::ResourceType::Audio) {
                            resourceText = "[音频]";
                        }

                        QLabel *resLabel = new QLabel(resourceText);
                        resLabel->setCursor(Qt::PointingHandCursor);

                        // 类似文本，允许垂直扩展
                        resLabel->setSizePolicy(QSizePolicy::Preferred,
                                                QSizePolicy::Preferred);
                        resLabel->setMaximumWidth(estInnerWidth);
                        resLabel->setMinimumWidth(0);

                        bubbleLayout->addWidget(resLabel);
                        contentWidgets.push_back(resLabel);

                        // 安装事件过滤器
                        resLabel->installEventFilter(this);
                    }
                }
            },
            block);
    }

    // 使布局重新计算并更新 geometry
    // bubbleLayout->activate();
    // bubbleWidget->updateGeometry();
    // bubbleWidget->adjustSize();
    // mainLayout->activate();

    // // 在事件循环后再调整文本控件高度，以确保 width 已经设置并触发正确的换行
    // QTimer::singleShot(0, this, [this, estInnerWidth] {
    //     // 更新文本控件的最大宽度并调整尺寸（确保文本不会被截断）
    //     updateTextLabelsMaxWidth(estInnerWidth);

    //     updateGeometry();
    // });
}


bool MessageItemWidget::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        // 如果点击的是 bubbleWidget 或 bubble 内的某个子控件，都触发选中
        // 这里不区分具体哪个子控件：只要是被安装过滤器的控件就会到这里
        setSelected(!selected_);
        emit clicked(message_);

        // 不拦截事件：让控件继续收到事件（例如图片的默认响应）
        return false;
    }

    // 默认处理
    return QObject::eventFilter(watched, event);
}

void MessageItemWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // 若含图片，按当前 bubble 可用宽度重新缩放（保持比例）
    if (!originalPixmaps.isEmpty()) {
        int padding = bubbleLayout->contentsMargins().left() +
                      bubbleLayout->contentsMargins().right();
        int avail = qMax(40, bubbleWidget->width() - padding);

        // 限制最大宽度
        int targetW = std::min(avail, IMAGE_MAX_WIDTH);

        for (auto it = originalPixmaps.begin(); it != originalPixmaps.end();
             ++it) {
            QLabel *lbl = it.key();
            QPixmap orig = it.value();
            if (lbl && !orig.isNull()) {
                QPixmap scaled =
                    orig.scaled(targetW, IMAGE_MAX_HEIGHT, Qt::KeepAspectRatio,
                                Qt::SmoothTransformation);
                lbl->setPixmap(scaled);
                lbl->setFixedSize(scaled.size());
            }
        }
    }

    // 统一更新文本控件的最大宽度（确保文本在宽度变化时重新换行并计算高度）
    {
        int padding = bubbleLayout->contentsMargins().left() +
                      bubbleLayout->contentsMargins().right();
        int avail = qMax(40, bubbleWidget->width() - padding);
        updateTextLabelsMaxWidth(avail);
    }

    // 更新 QListWidgetItem 的 sizeHint（如果是在 QListWidget 中）
    QWidget *p = parentWidget();
    QListWidget *list = nullptr;
    while (p) {
        list = qobject_cast<QListWidget *>(p);
        if (list) {
            break;
        }
        p = p->parentWidget();
    }
    if (list) {
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem *it = list->item(i);
            if (list->itemWidget(it) == this) {
                it->setSizeHint(sizeHint());
                break;
            }
        }
    }

    updateGeometry();
}

bool MessageItemWidget::isClickOnContent(QPoint const &pos) const {
    return bubbleWidget->geometry().contains(pos);
}

void MessageItemWidget::mousePressEvent(QMouseEvent *event) {
    if (isClickOnContent(event->pos())) {
        // 切换选中：选中时让背景色变暗（而不是画 border）
        setSelected(!selected_);
        emit clicked(message_);
    }
    QWidget::mousePressEvent(event);
}

void MessageItemWidget::contextMenuEvent(QContextMenuEvent *event) {
    showContextMenu(event->globalPos());
}

void MessageItemWidget::showContextMenu(QPoint const &pos) {
    QMenu menu(this);

    bool isSelf = (message_.senderId == currentUser_.id);

    QAction *copyAction = menu.addAction(tr("复制"));
    QAction *replyAction = menu.addAction(tr("回复"));
    QAction *forwardAction = menu.addAction(tr("转发"));

    // 只有自己发的消息才能撤回
    QAction *revokeAction = nullptr;
    if (isSelf && !message_.revoked) {
        menu.addSeparator();
        revokeAction = menu.addAction(tr("撤回"));
    }

    // 复制
    connect(copyAction, &QAction::triggered, [this]() {
        QString allText;
        for (auto const &block: message_.content) {
            std::visit(
                [&allText](auto const &content_block) {
                    using T = std::decay_t<decltype(content_block)>;
                    if constexpr (std::is_same_v<T, core::TextContent>) {
                        allText +=
                            QString::fromStdString(content_block.text) + "\n";
                    }
                },
                block);
        }
        if (!allText.isEmpty()) {
            QApplication::clipboard()->setText(allText.trimmed());
        }
    });

    // 回复
    connect(replyAction, &QAction::triggered, [this]() {
        emit replyRequested(message_);
    });

    // 转发
    connect(forwardAction, &QAction::triggered, [this]() {
        emit forwardRequested(message_);
    });

    // 撤回
    if (revokeAction) {
        connect(revokeAction, &QAction::triggered, [this]() {
            emit revokeRequested(message_);
        });
    }

    menu.exec(pos);
}

void MessageItemWidget::setSelected(bool selected) {
    selected_ = selected;

    // 计算基础颜色（与 setMessageData 中使用的颜色必须一致）
    bool isSelf = (message_.senderId == currentUser_.id);
    QColor baseColor = isSelf ? QColor(200, 220, 255) : QColor(255, 255, 255);

    if (selected_) {
        // 将背景色调暗一点（same hue, lower brightness）
        QColor darker = baseColor.darker(115); // 115% -> 稍微暗一些
        QString style = QString("background-color: rgb(%1,%2,%3); "
                                "border-radius: 8px;")
                            .arg(darker.red())
                            .arg(darker.green())
                            .arg(darker.blue());
        bubbleWidget->setStyleSheet(style);
    } else {
        // 恢复基础颜色（不使用 border）
        QString style = QString("background-color: rgb(%1,%2,%3); "
                                "border-radius: 8px; ")
                            .arg(baseColor.red())
                            .arg(baseColor.green())
                            .arg(baseColor.blue());
        bubbleWidget->setStyleSheet(style);
    }
}

} // namespace chat
} // namespace wechat
