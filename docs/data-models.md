# Data Models - 数据模型与表结构

> C++ 数据模型、数据库表结构（Server/Client 共用）、Client 同步扩展

## 数据模型关系

```
User (id)
  │
  ├── 好友关系 ──→ User                     (contacts 模块，不在 core)
  │
  ├── 所属 ──→ Group (id, ownerId, memberIds)
  │                │
  │                ├── 私聊 = 2 人 Group
  │                └── 群聊 = N 人 Group
  │
  ├── 发送 ──→ Message (id:int64, senderId, chatId, content, timestamp)
  │                │
  │                ├── chatId 始终是 Group.id
  │                └── MessageContent = variant<monostate, Text, Resource>
  │                                                            │
  │                                                            └── resourceId 引用
  │                                                                客户端: 本地路径查找，miss 则远程 fetch
  │                                                                服务器: 独立资源存储服务管理
  │
  └── 发布 ──→ Moment (id:int64, authorId, text, imageIds, timestamp)
                  │
                  ├── likedBy: [userId...]
                  └── comments: [Comment (id:int64, authorId, text, timestamp)]
```

## C++ 数据模型

```cpp
// ── User: 身份 ──
struct User {
    std::string id;
};

// ── Group: 聊天容器 (私聊 = 2人, 群聊 = N人) ──
struct Group {
    std::string id;
    std::string ownerId;                // 私聊时为空
    std::vector<std::string> memberIds;
};

// ── MessageContent: 消息载荷 ──
struct TextContent { std::string text; };

// 资源大类
enum class ResourceType : uint8_t { Image, Video, Audio, File };

// 资源小类 (具体格式)
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
    std::string resourceId;     // 资源 ID，不存实际内容
    ResourceType type;
    ResourceSubtype subtype;
    ResourceMeta meta;
};

// ── 消息内容 = 内容块列表，支持图文混排 ──
using ContentBlock = std::variant<std::monostate, TextContent, ResourceContent>;
using MessageContent = std::vector<ContentBlock>;

// ── Message: 一条消息 ──
struct Message {
    int64_t id = 0;
    std::string senderId;
    std::string chatId;         // 始终是 Group.id
    int64_t replyTo = 0;       // 引用消息 id，0 = 无引用
    MessageContent content;     // 内容块列表
    int64_t timestamp;
    int64_t editedAt;           // 最后编辑时间，0 = 未编辑
    bool revoked;               // 是否已撤回
    uint32_t readCount;         // 已读人数（私聊: 0或1, 群聊: 0~N）
    int64_t updatedAt;          // 最后修改时间（编辑/撤回时更新），0 = 未修改
};

// ── Moment: 朋友圈动态 ──
struct Moment {
    int64_t id = 0;
    std::string authorId;
    std::string text;
    std::vector<std::string> imageIds;  // 图片资源 ID 列表
    int64_t timestamp;
    std::vector<std::string> likedBy;   // 点赞用户 ID 列表

    struct Comment {
        int64_t id = 0;
        std::string authorId;
        std::string text;
        int64_t timestamp;
    };
    std::vector<Comment> comments;
};
```

---

## 数据库表结构（Server/Client 共用）

> Server 和 Client 使用同一套表结构，资源只存 ID，不存文件内容

```sql
CREATE TABLE users (
    id TEXT PRIMARY KEY
);

CREATE TABLE groups_ (
    id TEXT PRIMARY KEY,
    owner_id TEXT,             -- 私聊时为空
    updated_at INTEGER DEFAULT 0 -- 群信息变更时更新，用于增量同步
);

CREATE TABLE group_members (
    group_id TEXT,
    user_id TEXT,
    joined_at INTEGER NOT NULL,    -- 加入时间
    removed INTEGER DEFAULT 0,     -- 0=在群, 1=已退出/被移除
    updated_at INTEGER DEFAULT 0,  -- 最后变更时间，用于增量同步
    PRIMARY KEY (group_id, user_id)
);

CREATE TABLE friendships (
    user_id_a TEXT,
    user_id_b TEXT,
    PRIMARY KEY (user_id_a, user_id_b)
);

CREATE TABLE messages (
    id INTEGER PRIMARY KEY,
    sender_id TEXT,
    chat_id TEXT NOT NULL,
    reply_to INTEGER DEFAULT 0, -- 引用消息 id，0 = 无引用
    content_data TEXT NOT NULL, -- 序列化的内容块列表
    timestamp INTEGER NOT NULL,
    edited_at INTEGER DEFAULT 0,
    revoked INTEGER DEFAULT 0,
    read_count INTEGER DEFAULT 0,
    updated_at INTEGER DEFAULT 0 -- 编辑/撤回时更新，用于增量同步
);

CREATE TABLE moments (
    id INTEGER PRIMARY KEY,
    author_id TEXT NOT NULL,
    text TEXT NOT NULL DEFAULT '',
    timestamp INTEGER NOT NULL,
    updated_at INTEGER DEFAULT 0
);

CREATE TABLE moment_images (
    moment_id INTEGER NOT NULL,
    image_id TEXT NOT NULL,        -- 资源 ID
    sort_order INTEGER DEFAULT 0,
    PRIMARY KEY (moment_id, image_id)
);

CREATE TABLE moment_likes (
    moment_id INTEGER NOT NULL,
    user_id TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    PRIMARY KEY (moment_id, user_id)
);

CREATE TABLE moment_comments (
    id INTEGER PRIMARY KEY,
    moment_id INTEGER NOT NULL,
    author_id TEXT NOT NULL,
    text TEXT NOT NULL,
    timestamp INTEGER NOT NULL
);

CREATE INDEX idx_group_members_user ON group_members(user_id);

CREATE INDEX idx_messages_chat ON messages(chat_id, timestamp);
CREATE INDEX idx_messages_reply ON messages(reply_to);
CREATE INDEX idx_messages_updated ON messages(chat_id, updated_at);

CREATE INDEX idx_moments_author ON moments(author_id, timestamp);
CREATE INDEX idx_moment_comments_moment ON moment_comments(moment_id);
```

### 资源管理

消息中的资源（图片、视频、文件等）只存 `resourceId`，实际文件独立管理：

```
客户端获取资源:
  1. 查本地固定路径: {cache_dir}/resources/{resourceId}
  2. 命中 → 直接使用
  3. 未命中 → 从 server fetch，存到本地路径

服务器存储资源:
  独立的资源存储服务，通过 resourceId 管理
  与消息表解耦，消息只引用 ID
```

### Server vs Client 差异

| 维度 | Server | Client |
|------|--------|--------|
| 表结构 | 共用 | 共用 + 同步扩展表 |
| 数据范围 | 全量，所有用户 | 子集，仅当前用户相关 |
| 资源文件 | 独立资源存储服务 | 本地缓存，miss 则远程 fetch |
| 权威性 | 数据权威来源 | 缓存，可从 server 重建 |

---


---

## 类型映射表

**ContentBlock type:**

| type | variant 类型 |
|:---:|---|
| 0 | monostate |
| 1 | TextContent |
| 2 | ResourceContent |

**ResourceType (大类):**

| 值 | 类型 |
|:---:|---|
| 0 | Image |
| 1 | Video |
| 2 | Audio |
| 3 | File |

**ResourceSubtype (小类):**

| 值 | 类型 | 大类 |
|:---:|---|---|
| 0 | Png | Image |
| 1 | Jpeg | Image |
| 2 | Gif | Image |
| 3 | Webp | Image |
| 4 | Bmp | Image |
| 5 | Mp4 | Video |
| 6 | Avi | Video |
| 7 | Mkv | Video |
| 8 | Webm | Video |
| 9 | Mp3 | Audio |
| 10 | Wav | Audio |
| 11 | Ogg | Audio |
| 12 | Flac | Audio |
| 13 | Aac | Audio |
| 14 | Pdf | File |
| 15 | Doc | File |
| 16 | Xls | File |
| 17 | Zip | File |
| 18 | Unknown | File |
