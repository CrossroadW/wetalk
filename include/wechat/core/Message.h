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

// ── 资源大类 ──
enum class ResourceType : uint8_t {
    Image,
    Video,
    Audio,
    File
};

// ── 资源小类 (具体格式) ──
enum class ResourceSubtype : uint8_t {
    // Image
    Png, Jpeg, Gif, Webp, Bmp,
    // Video
    Mp4, Avi, Mkv, Webm,
    // Audio
    Mp3, Wav, Ogg, Flac, Aac,
    // File
    Pdf, Doc, Xls, Zip, Unknown
};

struct ResourceMeta {
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
    ResourceSubtype subtype;
    ResourceMeta meta;
};

// ── 消息内容 = 内容块列表 ──

using ContentBlock = std::variant<std::monostate, TextContent, ResourceContent>;
using MessageContent = std::vector<ContentBlock>;

// ── 消息 ──

struct Message {
    int64_t id = 0;
    std::string senderId;
    std::string chatId;         // 始终是 Group.id
    int64_t replyTo = 0;       // 引用消息 id，0 = 无引用
    MessageContent content;     // 内容块列表，支持图文混排
    int64_t timestamp;
    int64_t editedAt;           // 最后编辑时间，0 = 未编辑
    bool revoked;               // 是否已撤回
    uint32_t readCount;         // 已读人数
    int64_t updatedAt;          // 最后修改时间（编辑/撤回时更新），0 = 未修改
};

} // namespace core
} // namespace wechat
