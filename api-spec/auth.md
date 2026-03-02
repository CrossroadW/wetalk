# 认证 API

## 1. 生成二维码登录会话

### 客户端请求

```json
{
  "type": "qr_login_init",
  "data": {}
}
```

### 服务器响应

```json
{
  "type": "qr_login_init",
  "success": true,
  "data": {
    "session_id": "string",    // 会话 ID
    "qr_url": "string",        // 二维码 URL
    "expires_at": number       // 过期时间戳（秒）
  }
}
```

### 服务器推送（当用户扫码确认后）

```json
{
  "type": "qr_confirmed",
  "success": true,
  "data": {
    "user_id": number,
    "username": "string",
    "token": "string"          // 认证 token
  }
}
```

### 说明

- 客户端收到响应后，应使用 `qr_url` 生成二维码显示
- 客户端保持 WebSocket 连接，等待 `qr_confirmed` 推送
- 收到推送后，保存 `token` 到本地文件
- 二维码有效期 5 分钟

## 2. 验证 Token

### 客户端请求

```json
{
  "type": "verify_token",
  "data": {
    "token": "string"          // 必填：要验证的 token
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "verify_token",
  "success": true,
  "data": {
    "user_id": number,
    "username": "string"
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "verify_token",
  "success": false,
  "data": {
    "message": "string"        // 错误信息
  }
}
```

### 说明

- 用于验证本地保存的 token 是否仍然有效
- 验证成功后，客户端可以直接进入主界面
- 验证失败应清除本地 token，显示二维码登录

## 3. 登出

### 客户端请求

```json
{
  "type": "logout",
  "data": {}
}
```

### 服务器响应

```json
{
  "type": "logout",
  "success": true,
  "data": {}
}
```

### 说明

- 登出后，服务器会清除该 token
- 客户端应删除本地保存的 token
- 下次启动需要重新扫码登录
