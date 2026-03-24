# 数据库表结构

> Server 和 Client 使用同一套表结构。C++ 类型见 [cpp-types.md](./cpp-types.md)

## 表结构

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    token TEXT                          -- NULL = 未登录
);

CREATE TABLE groups_ (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_id INTEGER,                   -- 私聊时为 0
    version INTEGER DEFAULT 0           -- 群信息变更时更新，用于增量同步
);

CREATE TABLE group_members (
    group_id INTEGER,
    user_id INTEGER,
    joined_at INTEGER NOT NULL,
    removed INTEGER DEFAULT 0,          -- 0=在群, 1=已退出/被移除
    version INTEGER DEFAULT 0,          -- 最后变更时间，用于增量同步
    PRIMARY KEY (group_id, user_id)
);

CREATE TABLE friendships (
    user_id_a INTEGER,
    user_id_b INTEGER,
    PRIMARY KEY (user_id_a, user_id_b)
);

CREATE TABLE messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    chat_seq INTEGER NOT NULL,          -- per-chat 单调自增序号（每个 chat_id 从 1 开始）
    sender_id INTEGER,
    chat_id INTEGER NOT NULL,
    reply_to INTEGER DEFAULT 0,         -- 引用消息 id，0 = 无引用
    content_data TEXT NOT NULL,         -- 序列化的内容块列表
    revoked INTEGER DEFAULT 0,
    read_count INTEGER DEFAULT 0,
    version INTEGER DEFAULT 0           -- 全局单调递增版本号（编辑/撤回时更新）
);

CREATE TABLE moments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    author_id INTEGER NOT NULL,
    text TEXT NOT NULL DEFAULT '',
    timestamp INTEGER NOT NULL,
    version INTEGER DEFAULT 0
);

CREATE TABLE moment_images (
    moment_id INTEGER NOT NULL,
    image_id TEXT NOT NULL,             -- 资源 ID
    sort_order INTEGER DEFAULT 0,
    PRIMARY KEY (moment_id, image_id)
);

CREATE TABLE moment_likes (
    moment_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    PRIMARY KEY (moment_id, user_id)
);

CREATE TABLE moment_comments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    moment_id INTEGER NOT NULL,
    author_id INTEGER NOT NULL,
    text TEXT NOT NULL,
    timestamp INTEGER NOT NULL
);
```

## 索引

```sql
CREATE INDEX idx_group_members_user ON group_members(user_id);

CREATE INDEX idx_messages_chat_seq ON messages(chat_id, chat_seq);
CREATE INDEX idx_messages_version  ON messages(chat_id, version);
CREATE INDEX idx_messages_reply    ON messages(reply_to);

CREATE INDEX idx_moments_author          ON moments(author_id);
CREATE INDEX idx_moment_comments_moment  ON moment_comments(moment_id);
```

## 资源管理

消息中的资源（图片、视频、文件等）只存 `resourceId`，实际文件独立管理：

```
客户端获取资源：
  1. 查本地路径 {cache_dir}/resources/{resourceId}
  2. 命中 → 直接使用；未命中 → 从 server fetch，存到本地

服务器存储资源：
  独立资源服务，通过 resourceId 管理，与消息表解耦
```

## Server vs Client 差异

| 维度 | Server | Client |
|------|--------|--------|
| 表结构 | 本文件定义 | 同上 + 同步扩展表（见 data-cache-mechanism.md）|
| 数据范围 | 全量 | 仅当前用户相关的子集 |
| 资源文件 | 独立资源存储服务 | 本地缓存，miss 则远程 fetch |
| 权威性 | 数据权威来源 | 缓存，可从 server 重建 |

## chat_seq 分配规则

服务端写入消息时自动分配：

```sql
-- 取当前聊天的最大 seq + 1
INSERT INTO messages (chat_seq, ...)
VALUES ((SELECT COALESCE(MAX(chat_seq), 0) + 1 FROM messages WHERE chat_id = ?), ...)
```
