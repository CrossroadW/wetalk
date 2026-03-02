# 联系人 API

## 1. 获取好友列表

### 客户端请求

```json
{
  "type": "get_friends",
  "data": {}
}
```

### 服务器响应（成功）

```json
{
  "type": "get_friends",
  "success": true,
  "data": {
    "friends": [User]          // User 数组
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "get_friends",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 返回当前用户的所有好友
- User 对象格式见 [common.md](common.md)
- 需要先验证 token

## 2. 添加好友

### 客户端请求

```json
{
  "type": "add_friend",
  "data": {
    "friend_id": number        // 必填：要添加的好友 ID
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "add_friend",
  "success": true,
  "data": {}
}
```

### 服务器响应（失败）

```json
{
  "type": "add_friend",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 添加好友是双向的（A 添加 B，B 也会有 A 作为好友）
- 如果已经是好友，操作会被忽略（不会报错）
- 需要先验证 token
