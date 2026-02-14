#include "MessageUtils.h"
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

} // namespace chat
} // namespace wechat