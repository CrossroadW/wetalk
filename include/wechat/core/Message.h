#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace wechat {
namespace core {

// ── 内容块类型 ──

struct TextContent {
    std::string text;
};

enum class ResourceType : uint8_t {
    Image,
    Video,
    Audio,
    File
};

struct ResourceMeta {
    std::string mimeType;                       // "image/png", "application/pdf"
    std::size_t size;                            // 文件大小 (bytes)
    std::string filename;                        // 原始文件名
    std::map<std::string, std::string> extra;    // 扩展元信息
    // Image: {"width": "800", "height": "600"}
    // Audio: {"duration": "30"}
    // Video: {"width": "1920", "height": "1080", "duration": "120"}
};

struct ResourceContent {
    std::string resourceId;     // 服务器资源 ID，本地通过固定目录映射
    ResourceType type;
    ResourceMeta meta;
};

// ── 消息内容 = 内容块列表 ──

using ContentBlock = std::variant<std::monostate, TextContent, ResourceContent>;
using MessageContent = std::vector<ContentBlock>;

// ── 消息 ──

struct Message {
    std::string id;
    std::string senderId;
    std::string chatId;         // 始终是 Group.id
    std::string replyTo;        // 引用消息 id，空则无引用
    MessageContent content;     // 内容块列表，支持图文混排
    int64_t timestamp;
    int64_t editedAt;           // 最后编辑时间，0 = 未编辑
    bool revoked;               // 是否已撤回
    uint32_t readCount;         // 已读人数
};

} // namespace core
} // namespace wechat
