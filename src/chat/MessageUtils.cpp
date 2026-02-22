#include "MessageUtils.h"

#include <wechat/core/AppPaths.h>

#include <QFileInfo>

namespace wechat {
namespace chat {

// 辅助函数：提取文本内容
QString extractTextFromContent(core::MessageContent const &content) {
    QString text;
    for (auto const &block: content) {
        std::visit(
            [&text](auto const &content_block) {
                using T = std::decay_t<decltype(content_block)>;
                if constexpr (std::is_same_v<T, core::TextContent>) {
                    text += QString::fromStdString(content_block.text);
                } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
                    // 为资源类型显示描述性文字
                    if (content_block.type == core::ResourceType::Image) {
                        text += "[图片]";
                    } else if (content_block.type ==
                               core::ResourceType::Video) {
                        text += "[视频]";
                    } else if (content_block.type ==
                               core::ResourceType::Audio) {
                        text += "[音频]";
                    } else {
                        text += "[文件]";
                    }
                }
                // std::monostate 不产生文本
            },
            block);
    }
    return text;
}

// 辅助函数：获取资源路径
QString getResourcePath(core::ResourceContent const &resource) {
    auto resourceId = resource.resourceId;
    if (resourceId.empty()) return {};

    // 如果 resourceId 已经是绝对路径，直接返回
    if (resourceId[0] == '/' || resourceId.find(":/") != std::string::npos ||
        resourceId.find(":\\") != std::string::npos) {
        return QString::fromStdString(resourceId);
    }

    // 通过 AppPaths 拼接: {dataDir}/resources/{resourceId}
    auto path = QString::fromStdString(core::AppPaths::resourcePath(resourceId));
    if (QFileInfo::exists(path)) {
        return path;
    }

    // fallback: 返回原始 resourceId
    return QString::fromStdString(resourceId);
}

} // namespace chat
} // namespace wechat
