# C++ 数据类型

> 对应 `include/wechat/core/` 下的头文件定义。数据库表结构见 [data-models.md](./data-models.md)

## 关系概览

```
User  ──好友──▶  User
  └──所属──▶  Group (私聊=2人, 群聊=N人)
                └──发送──▶  Message (chatId = Group.id)
                              └──内容──▶  ContentBlock (Text | Resource)
User  ──发布──▶  Moment
```

## 类型定义

```cpp
// ── User ──
struct User {
    int64_t id = 0;
    std::string username;
    std::string password;
    std::string token;          // 当前会话 token，登录后填充
};

// ── Group ──
struct Group {
    int64_t id = 0;
    int64_t ownerId = 0;        // 私聊时为 0
    std::vector<int64_t> memberIds;
};

// ── MessageContent ──

struct TextContent { std::string text; };

enum class ResourceType : uint8_t { Image, Video, Audio, File };

enum class ResourceSubtype : uint8_t {
    Png, Jpeg, Gif, Webp, Bmp,          // Image
    Mp4, Avi, Mkv, Webm,                // Video
    Mp3, Wav, Ogg, Flac, Aac,           // Audio
    Pdf, Doc, Xls, Zip, Unknown         // File
};

struct ResourceMeta {
    std::size_t size;                            // 文件大小 (bytes)
    std::string filename;                        // 原始文件名
    std::map<std::string, std::string> extra;    // 扩展元信息
    // Image: {"width": "800", "height": "600"}
    // Video: {"width": "1920", "height": "1080", "duration": "120"}
    // Audio: {"duration": "30"}
};

struct ResourceContent {
    std::string resourceId;
    ResourceType type;
    ResourceSubtype subtype;
    ResourceMeta meta;
};

using ContentBlock   = std::variant<std::monostate, TextContent, ResourceContent>;
using MessageContent = std::vector<ContentBlock>;

// ── Message ──
struct Message {
    int64_t id = 0;
    int32_t chatSeq = 0;        // per-chat 单调自增序号（分页用）
    int64_t senderId = 0;
    int64_t chatId = 0;         // 始终是 Group.id
    int64_t replyTo = 0;        // 引用消息 id，0 = 无引用
    MessageContent content;
    int64_t timestamp = 0;
    int64_t editedAt = 0;       // 最后编辑时间，0 = 未编辑
    bool revoked = false;
    uint32_t readCount = 0;
    int64_t version = 0;        // 全局单调递增版本号（编辑/撤回时更新）
};

// ── Moment ──
struct Moment {
    int64_t id = 0;
    int64_t authorId = 0;
    std::string text;
    std::vector<std::string> imageIds;
    int64_t timestamp = 0;
    std::vector<int64_t> likedBy;

    struct Comment {
        int64_t id = 0;
        int64_t authorId = 0;
        std::string text;
        int64_t timestamp = 0;
    };
    std::vector<Comment> comments;
};
```

## 类型映射（序列化用）

**ContentBlock type 字段：**

| 值 | 类型 |
|:--:|------|
| 0 | monostate（占位） |
| 1 | TextContent |
| 2 | ResourceContent |

**ResourceType：**

| 值 | 类型 |
|:--:|------|
| 0 | Image |
| 1 | Video |
| 2 | Audio |
| 3 | File |

**ResourceSubtype：**

| 值 | 类型 | 大类 |
|:--:|------|------|
| 0–4 | Png/Jpeg/Gif/Webp/Bmp | Image |
| 5–8 | Mp4/Avi/Mkv/Webm | Video |
| 9–13 | Mp3/Wav/Ogg/Flac/Aac | Audio |
| 14–18 | Pdf/Doc/Xls/Zip/Unknown | File |
