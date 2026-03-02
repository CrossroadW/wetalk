# WeTalk Backend 快速开始

## 1. 安装依赖

```bash
cd backend
uv sync
```

## 2. 初始化测试数据

```bash
uv run python init_test_data.py
```

这会创建以下测试用户:
- alice
- bob
- charlie
- david
- eve

并添加一些好友关系。

## 3. 启动服务器

```bash
uv run python main.py
```

服务器将在 `http://localhost:8000` 启动。

## 4. 测试 WebSocket API

在另一个终端运行:

```bash
uv run python test_websocket.py
```

测试脚本会:
1. 连接到 WebSocket
2. 请求生成二维码
3. 显示二维码 URL
4. 等待你在浏览器中打开 URL 并输入用户名
5. 收到登录确认后，测试其他 API（好友列表、消息等）

## 5. 手动测试二维码登录

1. 启动服务器
2. 运行测试脚本，它会显示类似这样的 URL:
   ```
   http://localhost:8000/qr-login?session=abc123...
   ```
3. 在浏览器中打开这个 URL
4. 输入用户名（如 `alice`）
5. 点击"确认登录"
6. 测试脚本会收到推送通知并显示 token

## 6. 在客户端中使用

C++ 客户端需要:

1. **生成二维码**: 使用 zxing 库将 `qr_url` 转换为二维码图片
2. **WebSocket 连接**: 使用 Qt WebSocket 连接到 `ws://localhost:8000/ws`
3. **发送/接收 JSON**: 所有消息都是 JSON 格式
4. **Token 持久化**: 将 token 保存到本地文件，下次启动时使用 `verify_token` 验证

## WebSocket 消息示例

### 发送消息

```json
{
  "type": "send_message",
  "data": {
    "chat_id": 1,
    "content_data": "{\"blocks\":[{\"type\":\"text\",\"text\":\"Hello\"}]}",
    "reply_to": 0
  }
}
```

### 接收响应

```json
{
  "type": "send_message",
  "success": true,
  "data": {
    "message": {
      "id": 1,
      "sender_id": 1,
      "chat_id": 1,
      "content_data": "...",
      "updated_at": 1234567890
    }
  }
}
```

## 故障排除

### 端口被占用

如果 8000 端口被占用，修改 `main.py` 最后一行:

```python
uvicorn.run(app, host="0.0.0.0", port=8001)  # 改为其他端口
```

### 数据库损坏

删除数据库文件重新初始化:

```bash
rm wetalk.db
uv run python main.py  # 会自动创建新数据库
uv run python init_test_data.py  # 重新添加测试数据
```

### WebSocket 连接失败

确保:
1. 服务器正在运行
2. 防火墙没有阻止 8000 端口
3. 使用 `ws://` 而不是 `wss://`（本地开发不需要 SSL）
