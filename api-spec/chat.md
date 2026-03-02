# 聊天 API

## 1. 获取消息列表

### 客户端请求

```json
{
  "type": "get_messages",
  "data": {
    "chat_id": number,         // 必填：会话 ID
    "before_id": number,       // 可选：获取此 ID 之前的消息（默认 0）
    "limit": number            // 可选：最多返回多少条（默认 50）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "get_messages",
  "success": true,
  "data": {
    "messages": [Message]      // Message 数组，按 ID 降序
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "get_messages",
  "success": false,
  "data": {
    "message": "string"        // 错误信息（如"未登录"）
  }
}
```

### 说明

- `before_id` 为 0 时，获取最新的消息
- `before_id` 不为 0 时，获取该 ID 之前的消息（用于分页加载）
- 返回的消息按 ID 降序排列（最新的在前）
- 需要先验证 token

## 2. 发送消息

### 客户端请求

```json
{
  "type": "send_message",
  "data": {
    "chat_id": number,         // 必填：会话 ID
    "content_data": "string",  // 必填：消息内容（JSON 字符串）
    "reply_to": number         // 可选：回复的消息 ID（默认 0）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "send_message",
  "success": true,
  "data": {
    "message": Message         // 新创建的消息对象
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "send_message",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- `content_data` 是 JSON 字符串，格式见 [common.md](common.md) 中的 Message 定义
- `reply_to` 为 0 表示不回复任何消息
- 发送成功后，服务器返回完整的消息对象（包含 ID、时间戳等）
- 需要先验证 token

## 3. 编辑消息

### 客户端请求

```json
{
  "type": "edit_message",
  "data": {
    "msg_id": number,          // 必填：要编辑的消息 ID
    "content_data": "string"   // 必填：新的消息内容
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "edit_message",
  "success": true,
  "data": {}
}
```

### 服务器响应（失败）

```json
{
  "type": "edit_message",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 只能编辑自己发送的消息
- 编辑后，消息的 `updated_at` 字段会更新
- 需要先验证 token

## 4. 撤回消息

### 客户端请求

```json
{
  "type": "revoke_message",
  "data": {
    "msg_id": number           // 必填：要撤回的消息 ID
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "revoke_message",
  "success": true,
  "data": {}
}
```

### 服务器响应（失败）

```json
{
  "type": "revoke_message",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 只能撤回自己发送的消息
- 撤回后，消息的 `revoked` 字段变为 1
- 撤回的消息仍然存在于数据库中，但客户端应显示为"已撤回"
- 需要先验证 token
