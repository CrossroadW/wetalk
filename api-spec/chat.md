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

## 5. 向下同步（新消息）

### 客户端请求

```json
{
  "type": "fetch_after",
  "data": {
    "chat_id": number,         // 必填：会话 ID
    "after_id": number,        // 可选：获取此 ID 之后的消息（默认 0）
    "limit": number            // 可选：最多返回多少条（默认 50）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "fetch_after",
  "success": true,
  "data": {
    "messages": [Message],     // Message 数组，按 ID 升序
    "has_more": boolean        // 是否还有更多消息
  }
}
```

### 说明

- `after_id = 0`：返回最新的 limit 条消息（从末尾倒数），升序返回
- `after_id > 0`：返回 id > after_id 的前 limit 条消息，升序返回
- 用于初始加载和接收新消息后的增量同步

## 6. 向上翻页（历史消息）

### 客户端请求

```json
{
  "type": "fetch_before",
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
  "type": "fetch_before",
  "success": true,
  "data": {
    "messages": [Message],     // Message 数组，按 ID 升序
    "has_more": boolean        // 是否还有更早的消息
  }
}
```

### 说明

- `before_id = 0`：返回最早的 limit 条消息，升序返回
- `before_id > 0`：返回 id < before_id 的最后 limit 条消息，升序返回
- 用于向上滚动加载历史消息

## 7. 增量更新

### 客户端请求

```json
{
  "type": "fetch_updated",
  "data": {
    "chat_id": number,         // 必填：会话 ID
    "start_id": number,        // 必填：范围起始 ID（含）
    "end_id": number,          // 必填：范围结束 ID（含）
    "updated_at": number,      // 可选：只返回此时间戳之后有更新的消息（默认 0）
    "limit": number            // 可选：最多返回多少条（默认 50）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "fetch_updated",
  "success": true,
  "data": {
    "messages": [Message],     // 有变化的消息，按 ID 升序
    "has_more": boolean
  }
}
```

### 说明

- 用于检查已缓存消息范围内是否有撤回、编辑、已读数变化
- `updated_at = 0` 返回范围内所有消息
- `updated_at > 0` 只返回 updated_at 字段大于该值的消息

## 8. 标记已读

### 客户端请求

```json
{
  "type": "mark_read",
  "data": {
    "chat_id": number,         // 必填：会话 ID
    "last_message_id": number  // 必填：已读到的最后一条消息 ID
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "mark_read",
  "success": true,
  "data": {}
}
```

### 说明

- 将 chat_id 中 id ≤ last_message_id 且非自己发送的消息 read_count +1
- 需要先验证 token

## 9. 服务器推送：新消息

当群组中有其他成员发送消息时，服务器主动推送给所有在线成员。

```json
{
  "type": "new_message",
  "success": true,
  "data": {
    "message": Message         // 新消息对象
  }
}
```

### 说明

- 客户端收到推送后，应将消息插入本地缓存并刷新 UI
- 发送者本人不会收到自己消息的推送（通过 `send_message` 响应获取）
