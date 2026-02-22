#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
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

/// ResourceSubtype → 文件扩展名（含点号）
constexpr std::string_view toExtension(ResourceSubtype subtype) {
    switch (subtype) {
    case ResourceSubtype::Png:  return ".png";
    case ResourceSubtype::Jpeg: return ".jpg";
    case ResourceSubtype::Gif:  return ".gif";
    case ResourceSubtype::Webp: return ".webp";
    case ResourceSubtype::Bmp:  return ".bmp";
    case ResourceSubtype::Mp4:  return ".mp4";
    case ResourceSubtype::Avi:  return ".avi";
    case ResourceSubtype::Mkv:  return ".mkv";
    case ResourceSubtype::Webm: return ".webm";
    case ResourceSubtype::Mp3:  return ".mp3";
    case ResourceSubtype::Wav:  return ".wav";
    case ResourceSubtype::Ogg:  return ".ogg";
    case ResourceSubtype::Flac: return ".flac";
    case ResourceSubtype::Aac:  return ".aac";
    case ResourceSubtype::Pdf:  return ".pdf";
    case ResourceSubtype::Doc:  return ".doc";
    case ResourceSubtype::Xls:  return ".xls";
    case ResourceSubtype::Zip:  return ".zip";
    case ResourceSubtype::Unknown: return "";
    }
    return "";
}

/// 文件扩展名 → ResourceSubtype（小写，含点号）
constexpr ResourceSubtype subtypeFromExtension(std::string_view ext) {
    if (ext == ".png")  return ResourceSubtype::Png;
    if (ext == ".jpg" || ext == ".jpeg") return ResourceSubtype::Jpeg;
    if (ext == ".gif")  return ResourceSubtype::Gif;
    if (ext == ".webp") return ResourceSubtype::Webp;
    if (ext == ".bmp")  return ResourceSubtype::Bmp;
    if (ext == ".mp4")  return ResourceSubtype::Mp4;
    if (ext == ".avi")  return ResourceSubtype::Avi;
    if (ext == ".mkv")  return ResourceSubtype::Mkv;
    if (ext == ".webm") return ResourceSubtype::Webm;
    if (ext == ".mp3")  return ResourceSubtype::Mp3;
    if (ext == ".wav")  return ResourceSubtype::Wav;
    if (ext == ".ogg")  return ResourceSubtype::Ogg;
    if (ext == ".flac") return ResourceSubtype::Flac;
    if (ext == ".aac")  return ResourceSubtype::Aac;
    if (ext == ".pdf")  return ResourceSubtype::Pdf;
    if (ext == ".doc")  return ResourceSubtype::Doc;
    if (ext == ".xls")  return ResourceSubtype::Xls;
    if (ext == ".zip")  return ResourceSubtype::Zip;
    return ResourceSubtype::Unknown;
}

struct ResourceMeta {
    std::size_t size;                            // 文件大小 (bytes)
    std::string filename;                        // 原始文件名
    std::map<std::string, std::string> extra;    // 扩展元信息
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
    int64_t senderId = 0;
    int64_t chatId = 0;        // 始终是 Group.id
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
