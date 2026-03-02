# 朋友圈 API

## 1. 获取朋友圈

### 客户端请求

```json
{
  "type": "get_moments",
  "data": {
    "limit": number            // 可选：最多返回多少条（默认 20）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "get_moments",
  "success": true,
  "data": {
    "moments": [Moment]        // Moment 数组，按时间降序
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "get_moments",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 返回所有用户的朋友圈动态（按时间降序）
- Moment 对象格式见 [common.md](common.md)
- 需要先验证 token

## 2. 创建朋友圈

### 客户端请求

```json
{
  "type": "create_moment",
  "data": {
    "text": "string",          // 必填：文本内容（可为空字符串）
    "image_ids": ["string"]    // 可选：图片 ID 列表（默认空数组）
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "create_moment",
  "success": true,
  "data": {
    "id": number,
    "author_id": number,
    "text": "string",
    "image_ids": ["string"],
    "timestamp": number,
    "liked_by": [],
    "comments": []
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "create_moment",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- `text` 可以为空字符串，但必须提供该字段
- `image_ids` 是图片的唯一标识符列表
- 创建成功后返回完整的 Moment 对象
- 需要先验证 token
