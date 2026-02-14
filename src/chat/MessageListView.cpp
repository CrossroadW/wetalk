#include "MessageListView.h"
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

namespace wechat {
namespace chat {

// 辅助函数：提取文本内容
QString extractTextFromContent(const core::MessageContent& content) {
    QString text;
    for (const auto& block : content) {
        std::visit([&text](const auto& content_block) {
            using T = std::decay_t<decltype(content_block)>;
            if constexpr (std::is_same_v<T, core::TextContent>) {
                text += QString::fromStdString(content_block.text);
            } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
                // 为资源类型显示描述性文字
                if (content_block.type == core::ResourceType::Image) {
                    text += "[图片]";
                } else if (content_block.type == core::ResourceType::Video) {
                    text += "[视频]";
                } else if (content_block.type == core::ResourceType::Audio) {
                    text += "[音频]";
                } else {
                    text += "[文件]";
                }
            }
            // std::monostate 不产生文本
        }, block);
    }
    return text;
}

// 辅助函数：获取资源路径
QString getResourcePath(const core::ResourceContent& resource) {
    // 使用resourceId作为文件名，尝试从不同路径加载
    QString resourceFileName = QString::fromStdString(resource.resourceId);

    // 如果resourceId已经是完整路径，则直接返回
    if (resourceFileName.startsWith("/") || resourceFileName.contains(":/")) {
        return resourceFileName;
    }

    // 构建项目根目录的 img 目录路径
#ifdef PROJECT_ROOT_PATH
    QString imgPath = QString("%1/img/%2").arg(PROJECT_ROOT_PATH).arg(resourceFileName);
#else
    // Fallback: 尝试使用相对路径（从当前工作目录）
    QString imgPath = QString("img/%1").arg(resourceFileName);
#endif

    // 检查项目根目录下的 img 目录中是否存在该文件
    if (QFileInfo::exists(imgPath)) {
        return imgPath;
    }

    // 如果前面的路径都找不到文件，则返回原始资源ID
    return resourceFileName;
}

// MessageItemWidget 实现
MessageItemWidget::MessageItemWidget(QWidget *parent)
    : QWidget(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // 设置为 Preferred/Preferred 以让布局管理器决定
}

QSize MessageItemWidget::sizeHint() const {
    // 计算所需的高度以适应内容
    int height = 80; // 基础高度增加一点，留出足够空间

    // 估算内容宽度以确定高度需求
    QFontMetrics fm(font());

    // 根据消息内容计算实际需要的高度
    for (const auto& block : message_.content) {
        std::visit([&height, &fm, this](const auto& content_block) {
            using T = std::decay_t<decltype(content_block)>;
            if constexpr (std::is_same_v<T, core::TextContent>) {
                // 对于文本内容，可能需要多行
                QString text = QString::fromStdString(content_block.text);

                // 估算气泡的最大宽度（考虑到左右间距）
                int maxBubbleWidth = 300; // 设置合理的最大宽度

                int textHeight = fm.boundingRect(0, 0, maxBubbleWidth, 0, Qt::TextWordWrap, text).height();
                height += textHeight + 10; // Add padding for text
            } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
                // 对于图像资源，如果可能的话加载图像来确定尺寸
                if (content_block.type == core::ResourceType::Image) {
                    QString imagePath = getResourcePath(content_block);
                    QPixmap pixmap(imagePath);

                    if (!pixmap.isNull()) {
                        // 按比例缩放图像以适应宽度，设置合理的最大尺寸
                        int maxWidth = 250; // 限制最大宽度
                        int maxHeight = 200; // 限制最大高度

                        if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
                            pixmap = pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        }

                        height += pixmap.height() + 20; // Add image height plus padding
                    } else {
                        height += 40; // Height for error text
                    }
                } else {
                    height += 40; // Height for other resource types
                }
            }
            // std::monostate 不影响高度
        }, block);
    }

    // 确保至少有合理的最小高度
    int minHeight = 60;
    height = std::max(height, minHeight);

    // 返回固定宽度，让父组件处理布局
    int preferredWidth = 400; // 设定首选宽度

    return QSize(preferredWidth, height);
}

void MessageItemWidget::setMessageData(const core::Message& message, const core::User& currentUser) {
    message_ = message;
    currentUser_ = currentUser;

    // 重新计算大小提示
    updateGeometry();

    update();
}

void MessageItemWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 根据消息发送者决定样式
    bool isCurrentUser = message_.senderId == currentUser_.id;

    QRect contentRect = rect().adjusted(10, 5, -10, -5);

    // 绘制消息气泡 - 根据发送者调整位置
    QColor bubbleColor = isCurrentUser ? QColor(200, 220, 255) : QColor(240, 240, 240);

    // 计算气泡矩形：自己消息靠右，他人消息靠左
    QRect bubbleRect;
    int bubbleMargin = 70; // 增加边距以考虑头像空间
    int maxBubbleWidth = width() - bubbleMargin; // 确保不超出边界
    int minBubbleWidth = 100; // 最小宽度

    // 测量内容所需宽度，确保不超过可用宽度
    int contentWidth = 0;
    int totalHeight = 0;

    // 测量内容大小，限制最大宽度
    for (const auto& block : message_.content) {
        std::visit([&](const auto& content_block) {
            using T = std::decay_t<decltype(content_block)>;
            if constexpr (std::is_same_v<T, core::TextContent>) {
                QFontMetrics fm(painter.font());
                QString text = QString::fromStdString(content_block.text);
                QRect boundingRect = fm.boundingRect(0, 0, maxBubbleWidth - 30, 400, // -30 for left/right padding
                                                   Qt::TextWordWrap | Qt::AlignLeft, text);
                contentWidth = std::max(contentWidth, std::min(boundingRect.width() + 20, maxBubbleWidth - 20)); // 两边留边距，确保不超出
                totalHeight += boundingRect.height() + 5;
            } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
                if (content_block.type == core::ResourceType::Image) {
                    QString imagePath = getResourcePath(content_block);
                    QPixmap pixmap(imagePath);

                    if (!pixmap.isNull()) {
                        // 限制图片最大宽度为气泡可用宽度
                        int maxWidth = std::min(maxBubbleWidth - 20, 250); // 最大宽度限制
                        int maxHeight = 200; // 限制图片最大高度

                        if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
                            pixmap = pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        }

                        contentWidth = std::max(contentWidth, std::min(pixmap.width() + 20, maxBubbleWidth)); // 包含边距，确保不超出
                        totalHeight += pixmap.height() + 10;
                    } else {
                        QFontMetrics fm(painter.font());
                        QRect boundingRect = fm.boundingRect(0, 0, maxBubbleWidth - 30, 400,
                                                           Qt::TextWordWrap | Qt::AlignLeft,
                                                           QString("[无法加载图片: %1]").arg(imagePath));
                        contentWidth = std::max(contentWidth, std::min(boundingRect.width() + 20, maxBubbleWidth - 20));
                        totalHeight += boundingRect.height() + 5;
                    }
                } else {
                    QFontMetrics fm(painter.font());
                    QString resourceText;
                    if (content_block.type == core::ResourceType::Video) {
                        resourceText = "[视频]";
                    } else if (content_block.type == core::ResourceType::Audio) {
                        resourceText = "[音频]";
                    } else {
                        resourceText = "[文件]";
                    }
                    QRect boundingRect = fm.boundingRect(0, 0, maxBubbleWidth - 30, 400,
                                                       Qt::TextWordWrap | Qt::AlignLeft, resourceText);
                    contentWidth = std::max(contentWidth, std::min(boundingRect.width() + 20, maxBubbleWidth - 20));
                    totalHeight += boundingRect.height() + 5;
                }
            }
            // std::monostate 不做任何事情
        }, block);
    }

    // 确保内容宽度在合理范围内
    contentWidth = std::max(std::min(contentWidth, maxBubbleWidth), minBubbleWidth);

    // 根据是否为当前用户决定气泡位置
    if (isCurrentUser) {
        // 自己的消息靠右
        bubbleRect = QRect(width() - contentWidth - (bubbleMargin - 20), contentRect.top(),
                          contentWidth, contentRect.height());
    } else {
        // 对方消息靠左
        bubbleRect = QRect(bubbleMargin - 20, contentRect.top(),
                          contentWidth, contentRect.height());
    }

    // 绘制气泡背景
    painter.setBrush(bubbleColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(bubbleRect, 10, 10);

    // 绘制消息内容（文本和图像）
    int currentY = bubbleRect.top() + 10;  // 增加顶部边距

    for (const auto& block : message_.content) {
        std::visit([this, &painter, &currentY, &bubbleRect, isCurrentUser](const auto& content_block) {
            using T = std::decay_t<decltype(content_block)>;
            if constexpr (std::is_same_v<T, core::TextContent>) {
                // 绘制文本内容
                painter.setPen(Qt::black);

                int textWidth = std::min(bubbleRect.width() - 20, 400);  // 限制文本最大宽度
                QRect textRect = QRect(bubbleRect.left() + 10, currentY, textWidth, 25);

                QString text = QString::fromStdString(content_block.text);

                // 使用boundingRect来测量文本的实际高度
                QFontMetrics fm(painter.font());
                QRect boundingRect = fm.boundingRect(0, 0, textRect.width(), 0,
                                                   Qt::TextWordWrap | Qt::AlignLeft, text);

                // 绘制多行文本
                QRect actualTextRect = QRect(bubbleRect.left() + 10, currentY, textWidth, boundingRect.height());
                painter.drawText(actualTextRect, Qt::TextWordWrap | Qt::AlignVCenter, text);

                currentY += boundingRect.height() + 5; // Move down for next element with proper spacing
            } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
                // 处理资源内容
                if (content_block.type == core::ResourceType::Image) {
                    // 加载并绘制图像
                    QString imagePath = getResourcePath(content_block);
                    QPixmap pixmap(imagePath);

                    if (!pixmap.isNull()) {
                        // 限制图像最大尺寸以适应气泡，确保不超过可用空间
                        int maxWidth = std::min(bubbleRect.width() - 20, 250); // 限制最大宽度
                        int maxHeight = 200; // 限制最大高度

                        if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
                            pixmap = pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        }

                        QRect imageRect = QRect(bubbleRect.left() + 10, currentY, pixmap.width(), pixmap.height());
                        painter.drawPixmap(imageRect, pixmap);
                        currentY += pixmap.height() + 10; // Move down for next element with proper spacing
                    } else {
                        // 如果图像加载失败，显示文本提示
                        painter.setPen(Qt::red);
                        int textWidth = std::min(bubbleRect.width() - 20, 400); // 限制文本宽度
                        QRect textRect = QRect(bubbleRect.left() + 10, currentY, textWidth, 25);
                        painter.drawText(textRect, Qt::AlignVCenter, QString("[无法加载图片: %1]").arg(imagePath));
                        currentY += 30; // Move down for next element
                    }
                } else {
                    // 对于其他资源类型，显示描述文本
                    QString resourceText;
                    if (content_block.type == core::ResourceType::Video) {
                        resourceText = "[视频]";
                    } else if (content_block.type == core::ResourceType::Audio) {
                        resourceText = "[音频]";
                    } else {
                        resourceText = "[文件]";
                    }

                    painter.setPen(Qt::black);
                    int textWidth = std::min(bubbleRect.width() - 20, 400); // 限制文本宽度
                    QRect textRect = QRect(bubbleRect.left() + 10, currentY, textWidth, 25);
                    painter.drawText(textRect, Qt::AlignVCenter, resourceText);
                    currentY += 30; // Move down for next element
                }
            }
            // std::monostate 不做任何事情
        }, block);
    }

    // 绘制时间戳 - 根据消息方向放在合适的位置
    painter.setPen(Qt::gray);
    QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(message_.timestamp);
    QString timeStr = timestamp.toString("hh:mm");
    int timeWidth = 50;
    int timeHeight = 20;

    // 根据消息方向放置时间戳
    int timeX;
    if (isCurrentUser) {
        // 自己的消息，时间戳在气泡内部右侧
        timeX = std::min(bubbleRect.right() - timeWidth - 5, width() - timeWidth - 5); // 确保时间戳不超出边界
    } else {
        // 对方消息，时间戳在气泡内部左侧
        timeX = std::max(bubbleRect.left() + 5, 5); // 确保时间戳不超出边界
    }

    QRect timeRect = QRect(timeX,
                          std::max(currentY, bubbleRect.bottom() - timeHeight - 5),
                          timeWidth, timeHeight);
    painter.drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, timeStr);

    QWidget::paintEvent(event);
}

// MessageListView 实现
MessageListView::MessageListView(QWidget *parent)
    : QListWidget(parent) {
    setStyleSheet(
        "QListWidget {"
        "    background-color: white;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    border-bottom: 1px solid #eee;"
        "    padding: 5px;"
        "}"
    );

    // 设置平滑滚动和滚动条行为
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);  // 平滑滚动
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // 禁用水平滚动条
    setResizeMode(QListWidget::Adjust);  // 自适应大小
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(this, &QListWidget::itemClicked, this, &MessageListView::onItemClicked);
}

void MessageListView::addMessage(const core::Message& message, const core::User& currentUser) {
    // 创建自定义的消息项
    MessageItemWidget *messageWidget = new MessageItemWidget();

    // 先设置数据，这样sizeHint可以基于内容计算
    messageWidget->setMessageData(message, currentUser);

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

void MessageListView::onItemClicked(QListWidgetItem *item) {
    // 获取与该项关联的 widget
    MessageItemWidget *widget = qobject_cast<MessageItemWidget*>(itemWidget(item));
    if (widget) {
        emit messageSelected(widget->getMessage());  // 使用公共方法获取消息
    }
}

} // namespace chat
} // namespace wechat
