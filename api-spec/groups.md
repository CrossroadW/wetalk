# 群组 API

## 1. 获取群组列表

### 客户端请求

```json
{
  "type": "get_groups",
  "data": {}
}
```

### 服务器响应（成功）

```json
{
  "type": "get_groups",
  "success": true,
  "data": {
    "groups": [Group]          // Group 数组
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "get_groups",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 返回当前用户所在的所有群组（包括双人会话）
- Group 对象格式见 [common.md](common.md)
- 需要先验证 token

## 2. 创建群组

### 客户端请求

```json
{
  "type": "create_group",
  "data": {
    "member_ids": [number]     // 必填：成员 ID 列表（包括自己）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "create_group",
  "success": true,
  "data": {
    "id": number,
    "owner_id": number,
    "member_ids": [number]
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "create_group",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 如果 `member_ids` 只有 2 个人，`owner_id` 为 0（双人会话）
- 如果 `member_ids` 超过 2 个人，`owner_id` 为创建者 ID（群聊）
- `member_ids` 必须包含创建者自己的 ID
- 需要先验证 token
