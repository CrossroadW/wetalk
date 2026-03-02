# WeTalk Backend

基于 FastAPI + WebSocket + SQLite3 的微信后端服务。

## 📋 API 规范

**重要**: 前后端开发必须严格遵守 [../api-spec/](../api-spec/) 中定义的规范。

- 所有消息格式、字段定义、错误处理都在规范中明确定义
- 后端实现必须与规范一致
- 前端调用必须按规范发送请求
- 使用 API 规范可以让 AI 准确生成代码

**规范文档**:
- [README.md](../api-spec/README.md) - 总览和索引
- [common.md](../api-spec/common.md) - 通用消息格式、数据类型
- [auth.md](../api-spec/auth.md) - 认证 API
- [chat.md](../api-spec/chat.md) - 聊天 API
- [contacts.md](../api-spec/contacts.md) - 联系人 API
- [groups.md](../api-spec/groups.md) - 群组 API
- [moments.md](../api-spec/moments.md) - 朋友圈 API
- [errors.md](../api-spec/errors.md) - 错误处理
- [flows.md](../api-spec/flows.md) - 认证流程

## 特性

- **WebSocket 通信**: 所有客户端通信使用 WebSocket，支持实时推送
- **二维码登录**: 仅需用户名，无需密码（适合演示）
- **Token 认证**: 支持 token 持久化，记住登录状态
- **完整功能**: 聊天、联系人、群组、朋友圈

## 快速开始

### 安装依赖

使用 uv 管理依赖：

```bash
uv sync
```

### 运行服务器

```bash
uv run python main.py
```

服务器将在 `http://localhost:8000` 启动。

## API 文档

### WebSocket 连接

连接地址: `ws://localhost:8000/ws`

所有消息格式:

**请求**:
```json
{
  "type": "message_type",
  "data": { ... }
}
```

**响应**:
```json
{
  "type": "message_type",
  "success": true/false,
  "data": { ... }
}
```

### 认证相关

#### 1. 二维码登录初始化

桌面客户端请求生成二维码。

**请求**:
```json
{
  "type": "qr_login_init",
  "data": {}
}
```

**响应**:
```json
{
  "type": "qr_login_init",
  "success": true,
  "data": {
    "session_id": "abc123...",
    "qr_url": "http://localhost:8000/qr-login?session=abc123...",
    "expires_at": 1234567890
  }
}
```

客户端应该:
1. 使用 `qr_url` 生成二维码显示给用户
2. 保持 WebSocket 连接，等待 `qr_confirmed` 推送

**推送通知** (当用户扫码确认后):
```json
{
  "type": "qr_confirmed",
  "success": true,
  "data": {
    "user_id": 1,
    "username": "alice",
    "token": "xyz789..."
  }
}
```

#### 2. 验证 Token

检查已保存的 token 是否有效。

**请求**:
```json
{
  "type": "verify_token",
  "data": {
    "token": "xyz789..."
  }
}
```

**响应**:
```json
{
  "type": "verify_token",
  "success": true,
  "data": {
    "user_id": 1,
    "username": "alice"
  }
}
```

#### 3. 登出

**请求**:
```json
{
  "type": "logout",
  "data": {}
}
```

**响应**:
```json
{
  "type": "logout",
  "success": true
}
```

### 聊天相关

#### 1. 获取消息列表

**请求**:
```json
{
  "type": "get_messages",
  "data": {
    "chat_id": 1,
    "before_id": 0,
    "limit": 50
  }
}
```

**响应**:
```json
{
  "type": "get_messages",
  "success": true,
  "data": {
    "messages": [
      {
        "id": 1,
        "sender_id": 1,
        "chat_id": 1,
        "reply_to": 0,
        "content_data": "{...}",
        "revoked": 0,
        "read_count": 0,
        "updated_at": 1234567890
      }
    ]
  }
}
```

#### 2. 发送消息

**请求**:
```json
{
  "type": "send_message",
  "data": {
    "chat_id": 1,
    "content_data": "{...}",
    "reply_to": 0
  }
}
```

**响应**:
```json
{
  "type": "send_message",
  "success": true,
  "data": {
    "message": { ... }
  }
}
```

#### 3. 编辑消息

**请求**:
```json
{
  "type": "edit_message",
  "data": {
    "msg_id": 1,
    "content_data": "{...}"
  }
}
```

#### 4. 撤回消息

**请求**:
```json
{
  "type": "revoke_message",
  "data": {
    "msg_id": 1
  }
}
```

### 联系人相关

#### 1. 获取好友列表

**请求**:
```json
{
  "type": "get_friends",
  "data": {}
}
```

**响应**:
```json
{
  "type": "get_friends",
  "success": true,
  "data": {
    "friends": [
      { "id": 2, "username": "bob" }
    ]
  }
}
```

#### 2. 添加好友

**请求**:
```json
{
  "type": "add_friend",
  "data": {
    "friend_id": 2
  }
}
```

### 群组相关

#### 1. 获取群组列表

**请求**:
```json
{
  "type": "get_groups",
  "data": {}
}
```

**响应**:
```json
{
  "type": "get_groups",
  "success": true,
  "data": {
    "groups": [
      {
        "id": 1,
        "owner_id": 1,
        "member_ids": [1, 2, 3]
      }
    ]
  }
}
```

#### 2. 创建群组

**请求**:
```json
{
  "type": "create_group",
  "data": {
    "member_ids": [1, 2, 3]
  }
}
```

### 朋友圈相关

#### 1. 获取朋友圈

**请求**:
```json
{
  "type": "get_moments",
  "data": {
    "limit": 20
  }
}
```

**响应**:
```json
{
  "type": "get_moments",
  "success": true,
  "data": {
    "moments": [
      {
        "id": 1,
        "author_id": 1,
        "text": "Hello",
        "image_ids": [],
        "timestamp": 1234567890,
        "liked_by": [],
        "comments": []
      }
    ]
  }
}
```

#### 2. 创建朋友圈

**请求**:
```json
{
  "type": "create_moment",
  "data": {
    "text": "Hello World",
    "image_ids": []
  }
}
```

## 二维码登录流程

1. **桌面客户端**: 发送 `qr_login_init` 请求
2. **服务器**: 返回 `session_id` 和 `qr_url`
3. **桌面客户端**: 使用 zxing 生成二维码显示 `qr_url`
4. **手机用户**: 扫描二维码，打开网页
5. **网页**: 显示登录表单，用户输入用户名
6. **网页**: POST 到 `/api/qr-confirm`
7. **服务器**: 验证用户名，生成 token，推送 `qr_confirmed` 给桌面客户端
8. **桌面客户端**: 收到推送，保存 token，进入主界面

## 数据库

SQLite3 数据库文件: `wetalk.db`

表结构:
- `users` - 用户表
- `groups_` - 群组表
- `group_members` - 群组成员
- `friendships` - 好友关系
- `messages` - 消息表
- `moments` - 朋友圈
- `moment_images` - 朋友圈图片
- `moment_likes` - 朋友圈点赞
- `moment_comments` - 朋友圈评论
- `qr_sessions` - 二维码登录会话

## 开发

### 添加测试用户

可以直接操作数据库添加测试用户:

```bash
sqlite3 wetalk.db
```

```sql
INSERT INTO users (username, password) VALUES ('alice', '');
INSERT INTO users (username, password) VALUES ('bob', '');
INSERT INTO users (username, password) VALUES ('charlie', '');
```

注意: password 字段保留但不使用，可以为空字符串。

### 清空数据库

```bash
rm wetalk.db
```

重启服务器会自动重新创建。
