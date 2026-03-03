# 认证 API

## 1. 用户注册

### 客户端请求

```json
{
  "type": "register",
  "data": {
    "username": "string",  // 必填：用户名
    "password": "string"   // 必填：密码
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "register",
  "success": true,
  "data": {
    "user": {
      "id": number,
      "username": "string",
      "token": "string"
    }
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "register",
  "success": false,
  "data": {
    "message": "string"    // 错误信息（如"用户名已存在"）
  }
}
```

### 说明

- 注册成功后自动登录，WebSocket 连接绑定到新用户
- 保存返回的 `token` 到本地文件，用于下次自动登录

## 2. 用户登录

### 客户端请求

```json
{
  "type": "login",
  "data": {
    "username": "string",  // 必填：用户名
    "password": "string"   // 必填：密码
  }
}
```

### 服务器响应（成功）

```json
{
  "type": "login",
  "success": true,
  "data": {
    "user": {
      "id": number,
      "username": "string",
      "token": "string"    // 每次登录生成新 token
    }
  }
}
```

### 服务器响应（失败）

```json
{
  "type": "login",
  "success": false,
  "data": {
    "message": "string"    // 错误信息（如"用户名或密码错误"）
  }
}
```

### 说明

- 每次登录生成新 token，旧 token 失效
- 登录成功后保存新 token 到本地文件

## 3. 生成二维码登录会话

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

## 4. 验证 Token

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

## 5. 登出

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

## HTTP 端点

### GET /qr-login?session={session_id}

二维码扫码后显示的网页登录页面（手机浏览器打开）。

用户在页面中输入用户名，点击确认后调用 `/api/qr-confirm`。

### POST /api/qr-confirm

手机端确认登录（仅需用户名，无需密码）。

**请求体**:
```json
{
  "session_id": "string",
  "username": "string"
}
```

**响应（成功）**:
```json
{
  "success": true,
  "message": "登录成功"
}
```

**响应（失败）**:
```json
{
  "success": false,
  "detail": "string"   // 错误信息（如"二维码已过期"、"用户不存在"）
}
```

**说明**:
- 确认成功后，服务器向桌面客户端推送 `qr_confirmed` 消息
- 用户必须已存在（不支持通过扫码注册）
