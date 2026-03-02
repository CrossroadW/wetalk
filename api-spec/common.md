# 通用规范

## 连接信息

- **协议**: WebSocket
- **端点**: `ws://localhost:8000/ws`
- **消息格式**: JSON

## 通用消息格式

### 客户端请求

```json
{
  "type": "string",      // 必填：消息类型
  "data": {              // 必填：消息数据（可为空对象）
    // 具体字段根据消息类型而定
  }
}
```

### 服务器响应

```json
{
  "type": "string",      // 必填：消息类型（与请求相同）
  "success": boolean,    // 必填：操作是否成功
  "data": {              // 必填：响应数据（可为空对象）
    // 具体字段根据消息类型而定
  }
}
```

### 服务器推送

服务器可以主动推送消息给客户端（不需要客户端请求）：

```json
{
  "type": "string",      // 推送消息类型
  "success": boolean,    // 通常为 true
  "data": {              // 推送数据
    // 具体字段根据推送类型而定
  }
}
```

## 数据类型定义

### User

```json
{
  "id": number,          // 用户 ID
  "username": "string"   // 用户名
}
```

### Message

```json
{
  "id": number,          // 消息 ID
  "sender_id": number,   // 发送者 ID
  "chat_id": number,     // 会话 ID
  "reply_to": number,    // 回复的消息 ID（0 表示不回复）
  "content_data": "string",  // JSON 字符串，包含消息内容块
  "revoked": number,     // 是否撤回（0=否，1=是）
  "read_count": number,  // 已读人数
  "updated_at": number   // 更新时间戳（秒）
}
```

**content_data 格式示例**:

```json
{
  "blocks": [
    {
      "type": "text",
      "text": "Hello World"
    },
    {
      "type": "image",
      "image_id": "img_123",
      "width": 800,
      "height": 600
    },
    {
      "type": "link",
      "url": "https://example.com",
      "title": "Example"
    }
  ]
}
```

支持的 block 类型：text, image, file, link 等。

### Group

```json
{
  "id": number,          // 群组 ID
  "owner_id": number,    // 群主 ID（0 表示双人会话）
  "member_ids": [number] // 成员 ID 列表
}
```

**说明**:
- 如果 `member_ids` 只有 2 个人，`owner_id` 为 0（双人会话）
- 如果 `member_ids` 超过 2 个人，`owner_id` 为创建者 ID（群聊）

### Moment

```json
{
  "id": number,          // 朋友圈 ID
  "author_id": number,   // 作者 ID
  "text": "string",      // 文本内容
  "image_ids": ["string"],  // 图片 ID 列表
  "timestamp": number,   // 发布时间戳（秒）
  "liked_by": [number],  // 点赞用户 ID 列表
  "comments": [          // 评论列表
    {
      "id": number,
      "author_id": number,
      "text": "string",
      "timestamp": number
    }
  ]
}
```

## 字段约定

### 必填 vs 可选

- **必填**: 请求中必须包含该字段，否则返回错误
- **可选**: 请求中可以省略该字段，服务器使用默认值

### 默认值

文档中标注"默认 X"表示该字段可选，省略时使用默认值。

例如：`"limit": number  // 可选：最多返回多少条（默认 50）`

### 时间戳

所有时间戳字段使用 Unix 时间戳（秒），例如：`1709366400`

### ID 字段

所有 ID 字段使用正整数，0 通常表示"无"或"不存在"。
