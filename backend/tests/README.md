# 测试指南

## 运行测试

### 安装依赖

```bash
cd backend
uv sync
```

### 运行所有测试

```bash
uv run pytest
```

### 运行特定测试文件

```bash
uv run pytest tests/test_auth.py -v
```

### 运行特定测试函数

```bash
uv run pytest tests/test_auth.py::test_qr_login_init -v
```

### 显示详细输出

```bash
uv run pytest -v -s
```

## 测试框架特性

### 自动启动/停止后端服务

测试框架使用**同进程方案**，在独立线程中启动 uvicorn 服务器：

- ✅ 启动快速（约1秒）
- ✅ 易于控制和调试
- ✅ 不需要手动启动后端
- ✅ 测试完成自动清理

### Fixture 支持

- `backend_server` - session 级别，整个测试会话只启动一次
- `ws_client` - function 级别，每个测试函数都有独立的 WebSocket 连接
- `authenticated_client` - 提供已认证的客户端（自动获取 token）
