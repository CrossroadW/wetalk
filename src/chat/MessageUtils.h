#pragma once

#include <QString>
#include <wechat/core/Message.h>

namespace wechat {
namespace chat {

// 辅助函数：提取文本内容
QString extractTextFromContent(const core::MessageContent& content);

// 辅助函数：获取资源路径
QString getResourcePath(const core::ResourceContent& resource);

} // namespace chat
} // namespace wechat