# Core Architecture - 数据模型与系统设计

> 核心数据模型如何协作，后台简化为本地数据库

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
  └── 发送 ──→ Message (id, senderId, chatId, content, timestamp)
                  │
                  ├── chatId 始终是 Group.id
                  └── MessageContent = variant<monostate, Text, Image, File>
```

## 核心设计: 统一聊天模型

**所有聊天都是 Group，私聊是 2 人的 Group。**

```
私聊:  Group { id: "g0", ownerId: "", memberIds: [A, B] }
群聊:  Group { id: "g1", ownerId: A,  memberIds: [A, B, C] }
```

不需要 ChatType 枚举，`memberIds.size() == 2` 即私聊。

**私聊转群聊 = 原 Group 加人:**

```
A ↔ B 私聊中 (Group { id: "g0", members: [A, B] })
  │
  A 邀请 C
  │
  ├─ group.memberIds.push_back(C)   ← 原 Group 加人
  ├─ chatId 不变，还是 "g0"
  ├─ 历史消息全部保留，无需迁移
  └─ 私聊自然变成了群聊
```

## Core 数据模型

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

enum class ResourceType : uint8_t { Image, Video, Audio, File };

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

// ── 消息内容 = 内容块列表，支持图文混排 ──
using ContentBlock = std::variant<std::monostate, TextContent, ResourceContent>;
using MessageContent = std::vector<ContentBlock>;

// 示例: "看看这张图 [图片] 还有这个文件 [文件]"
// MessageContent = [
//     TextContent{"看看这张图"},
//     ResourceContent{id: "img1", type: Image, ...},
//     TextContent{"还有这个文件"},
//     ResourceContent{id: "file1", type: File, ...}
// ]

// ── Message: 一条消息 ──
struct Message {
    std::string id;
    std::string senderId;
    std::string chatId;         // 始终是 Group.id
    std::string replyTo;        // 引用消息 id，空则无引用（支持跨群引用）
    MessageContent content;     // 内容块列表
    int64_t timestamp;
    int64_t editedAt;           // 最后编辑时间，0 = 未编辑
    bool revoked;               // 是否已撤回
    uint32_t readCount;         // 已读人数（私聊: 0或1, 群聊: 0~N）
};
```

**不在 core 中的类型:**

| 类型 | 所属模块 | 原因 |
|------|---------|------|
| Friendship | contacts | 好友关系是社交图谱，不是核心数据 |
| 聊天列表项 | chat | 未读数、最后消息等属于 UI 状态 |
| 用户资料 (昵称/头像) | contacts | 属于展示层，不是身份本身 |

## 数据库设计 (SQLite)

```sql
-- 用户表
CREATE TABLE users (
    id TEXT PRIMARY KEY
);

-- 群组表 (私聊也是 Group)
CREATE TABLE groups_ (
    id TEXT PRIMARY KEY,
    owner_id TEXT  -- 私聊时为空
);

-- 群组成员 (多对多)
CREATE TABLE group_members (
    group_id TEXT,
    user_id TEXT,
    PRIMARY KEY (group_id, user_id)
);

-- 好友关系 (contacts 模块管理)
CREATE TABLE friendships (
    user_id_a TEXT,
    user_id_b TEXT,
    PRIMARY KEY (user_id_a, user_id_b)
);

-- 消息表
CREATE TABLE messages (
    id TEXT PRIMARY KEY,
    sender_id TEXT,
    chat_id TEXT NOT NULL,
    reply_to TEXT,                -- 引用消息 id，NULL 则无引用
    content_data TEXT NOT NULL,   -- JSON 数组，内容块列表
    timestamp INTEGER NOT NULL,
    edited_at INTEGER DEFAULT 0,  -- 最后编辑时间，0 = 未编辑
    revoked INTEGER DEFAULT 0,    -- 是否已撤回，0=否 1=是
    read_count INTEGER DEFAULT 0  -- 已读人数
);

CREATE INDEX idx_messages_chat ON messages(chat_id, timestamp);
CREATE INDEX idx_messages_reply ON messages(reply_to);
```

**content_data 格式 (JSON 数组，每个元素是一个内容块):**

```json
[
    {"type": 0, "data": null},                                          // monostate
    {"type": 1, "data": {"text": "看看这张图"}},                          // TextContent
    {"type": 2, "data": {"resourceId": "abc123", "type": 0, "meta": {   // ResourceContent
        "mimeType": "image/png", "size": 102400,
        "filename": "photo.png",
        "extra": {"width": "800", "height": "600"}
    }}}
]
```

**ContentBlock type 映射:**

| type | variant 类型 |
|:---:|---|
| 0 | monostate |
| 1 | TextContent |
| 2 | ResourceContent |

**ResourceType 映射:**

| 值 | 类型 |
|:---:|---|
| 0 | Image |
| 1 | Video |
| 2 | Audio |
| 3 | File |

## 模块职责

```
┌─────────────────────────────────────────────────┐
│                    wetalk (主程序)                │
│                  组装模块，启动 UI                 │
└──────┬─────────────┬──────────────┬─────────────┘
       │             │              │
 ┌─────▼─────┐ ┌────▼─────┐ ┌─────▼──────┐
 │   chat    │ │   auth   │ │  contacts  │    ← 业务模块
 │  聊天逻辑  │ │  登录登出  │ │ 好友/群组逻辑│
 └─────┬─────┘ └────┬─────┘ └─────┬──────┘
       │             │              │
 ┌─────▼─────────────▼──────────────▼──────┐
 │               storage                    │  ← 持久化层
 │       Repository 模式，操作 SQLite        │
 └─────────────────┬───────────────────────┘
                   │
 ┌─────────────────▼───────────────────────┐
 │                core                      │  ← 核心层
 │   User, Message, Group                   │
 │   MessageContent, EventBus, Result       │
 └─────────────────────────────────────────┘
```

## 核心操作流程

### 1. 发起私聊

```
用户操作: 点击好友发起聊天

contacts 模块:
  1. 查询是否已存在包含 {self, friend} 的 2 人 Group
     SELECT g.id FROM groups_ g
     JOIN group_members m1 ON g.id = m1.group_id
     JOIN group_members m2 ON g.id = m2.group_id
     WHERE m1.user_id = :self AND m2.user_id = :friend
     AND (SELECT COUNT(*) FROM group_members WHERE group_id = g.id) = 2
  2. 不存在 → 创建 Group { id: UUID, ownerId: "", memberIds: [self, friend] }

chat 模块:
  3. chatId = group.id
  4. 加载历史消息
     SELECT * FROM messages WHERE chat_id = :groupId ORDER BY timestamp
  5. 渲染聊天界面
```

### 2. 私聊转群聊 (邀人)

```
用户操作: 在聊天中点击邀请

contacts 模块:
  1. 获取当前 Group
  2. group.memberIds.push_back(invitee)
  3. group.ownerId = self (升级为群主)
  4. 持久化更新
  5. EventBus 发布 GroupMemberJoined 事件

chat 模块:
  6. chatId 不变
  7. 历史消息全部保留
  8. UI 从私聊样式切换为群聊样式
```

### 3. 发送消息

```
用户操作: 在聊天窗口输入文本和图片，点击发送

chat 模块:
  1. 构造 Message {
       id: UUID,
       senderId: self,
       chatId: group.id,
       replyTo: "" (或引用的消息 id),
       content: [
           TextContent{"看看这张图"},
           ResourceContent{resourceId: "img1", type: Image, meta: {...}}
       ],
       timestamp: now,
       editedAt: 0,
       revoked: false,
       readCount: 0
     }
  2. storage 持久化
     INSERT INTO messages (id, sender_id, chat_id, reply_to, content_data, timestamp)
     VALUES (:id, :self, :chatId, :replyTo, :contentJson, :ts)
  3. EventBus 发布 MessageSent{message}

UI 层 (订阅 EventBus):
  4. 追加消息到聊天界面
  5. 聊天列表更新最后一条消息
```

### 4. 编辑消息

```
用户操作: 长按自己的消息，选择编辑

chat 模块:
  1. 修改 message.content
  2. message.editedAt = now
  3. storage 更新
     UPDATE messages SET content_data = :newContent, edited_at = :now WHERE id = :msgId
  4. EventBus 发布 MessageEdited{message}

UI 层:
  5. 更新消息内容，显示"已编辑"标记
```

### 5. 撤回消息

```
用户操作: 长按自己的消息，选择撤回

chat 模块:
  1. message.revoked = true
  2. storage 更新
     UPDATE messages SET revoked = 1 WHERE id = :msgId
  3. EventBus 发布 MessageRevoked{messageId, chatId}

UI 层:
  4. 替换消息内容为"消息已撤回"
```

### 6. 引用消息

```
用户操作: 滑动消息选择引用，输入回复内容

chat 模块:
  1. 构造 Message { replyTo: 被引用消息的 id, ... }
  2. 正常发送流程

UI 层:
  3. 渲染时通过 replyTo 查询被引用消息
     SELECT * FROM messages WHERE id = :replyTo
  4. 在消息上方显示引用预览（支持跨群，因为 id 全局唯一）
```

### 7. 转发消息

```
用户操作: 长按消息，选择转发到某个群

chat 模块:
  1. 复制原消息内容，构造新 Message {
       id: 新 UUID,
       senderId: self (当前用户，不是原发送人),
       chatId: 目标 group.id,
       replyTo: "",
       content: 原消息的 content 副本,
       ...
     }
  2. 正常发送流程，无跨 Group 引用
```

### 8. 加载聊天列表

```
用户操作: 打开应用

chat 模块:
  1. 查询当前用户所在的所有 Group 中有消息的
     SELECT m.chat_id, MAX(m.timestamp) as last_ts
     FROM messages m
     JOIN group_members gm ON m.chat_id = gm.group_id
     WHERE gm.user_id = :self
     GROUP BY m.chat_id
     ORDER BY last_ts DESC

  2. 对每个 chatId，查最后一条消息
     SELECT * FROM messages WHERE chat_id = :id ORDER BY timestamp DESC LIMIT 1

  3. 未读数 (本地维护 last_read_timestamp)
     SELECT COUNT(*) FROM messages
     WHERE chat_id = :id AND timestamp > :last_read

  4. 判断私聊/群聊
     SELECT COUNT(*) FROM group_members WHERE group_id = :id
     -- count == 2 → 私聊样式, count > 2 → 群聊样式

  5. 组装为 chat 模块的 UI 数据 (不在 core 中)
     struct ChatListItem {
         std::string chatId;
         Message lastMessage;
         int unreadCount;
         bool isGroup;  // memberIds.size() > 2
     };
```

## Storage 层接口 (Repository 模式)

```cpp
// storage 模块内部定义，不在 core 中

class MessageRepository {
public:
    Result<void> save(Message const& msg);
    Result<Message> findById(std::string const& id);
    Result<std::vector<Message>> findByChat(
        std::string const& chatId,
        int64_t beforeTimestamp,
        std::size_t limit);
    Result<void> updateContent(std::string const& msgId,
        MessageContent const& content, int64_t editedAt);
    Result<void> revoke(std::string const& msgId);
    Result<void> incrementReadCount(std::string const& msgId);
};

class GroupRepository {
public:
    Result<void> save(Group const& group);
    Result<Group> findById(std::string const& id);
    Result<std::vector<Group>> findByUser(std::string const& userId);
    Result<std::optional<Group>> findPrivateChat(
        std::string const& userIdA, std::string const& userIdB);
    Result<void> addMember(std::string const& groupId, std::string const& userId);
    Result<void> removeMember(std::string const& groupId, std::string const& userId);
};

class UserRepository {
public:
    Result<void> save(User const& user);
    Result<User> findById(std::string const& id);
};

class FriendshipRepository {
public:
    Result<void> add(std::string const& userIdA, std::string const& userIdB);
    Result<void> remove(std::string const& userIdA, std::string const& userIdB);
    Result<bool> exists(std::string const& userIdA, std::string const& userIdB);
    Result<std::vector<std::string>> findFriends(std::string const& userId);
};
```

## EventBus 事件类型 (待定义到 Event.h)

```cpp
// 消息相关
struct MessageSent     { Message message; };
struct MessageReceived { Message message; };
struct MessageEdited   { Message message; };
struct MessageRevoked  { std::string messageId; std::string chatId; };
struct MessageRead     { std::string messageId; uint32_t readCount; };

// 群组相关
struct GroupCreated       { Group group; };
struct GroupMemberJoined  { std::string groupId; std::string userId; };
struct GroupMemberLeft    { std::string groupId; std::string userId; };

// 好友相关
struct FriendAdded   { std::string userId; };
struct FriendRemoved { std::string userId; };

// 用户相关
struct UserLoggedIn  { User user; };
struct UserLoggedOut {};

using Event = std::variant<
    std::monostate,
    MessageSent, MessageReceived, MessageEdited, MessageRevoked, MessageRead,
    GroupCreated, GroupMemberJoined, GroupMemberLeft,
    FriendAdded, FriendRemoved,
    UserLoggedIn, UserLoggedOut
>;
```

## 关键设计决策

| 决策 | 选择 | 理由 |
|------|------|------|
| ID 类型 | std::string (UUID) | 通用，不依赖数据库自增 |
| 模型间关联 | string id 引用 | 无循环依赖，可独立序列化 |
| 消息内容 | ContentBlock 列表 | 支持图文混排，每个 block 是 variant |
| 消息引用 | replyTo = message id | 全局唯一 UUID，天然支持跨群引用 |
| 消息转发 | 复制内容，senderId = 当前用户 | 无跨 Group 引用，简单直接 |
| 编辑/撤回 | editedAt + revoked 字段 | 在 Message 上直接标记 |
| 已读 | readCount 计数 | 私聊 0/1，群聊 0~N，不存具体用户列表 |
| 私聊 vs 群聊 | 统一为 Group | 私聊 = 2 人 Group，转群只需加人，无需迁移消息 |
| 聊天列表 | 从 messages 聚合 | 不是存储实体，是派生视图 |
| 好友关系 | contacts 模块管理 | 独立于聊天，社交图谱关系 |
| 后台 | SQLite | 当前简化为本地数据库，未来可替换 |
| core 不依赖 Qt | std::string | 纯 C++ 标准库类型 |
