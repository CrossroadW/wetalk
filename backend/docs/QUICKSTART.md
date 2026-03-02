# WeTalk Backend 快速开始

## 项目结构

```
backend/
├── main.py                    # 应用入口
├── core/                      # 核心模块
│   └── database.py           # 数据库初始化
├── managers/                  # 管理器模块
│   └── connection_manager.py # WebSocket 连接管理
├── routers/                   # 路由处理模块
│   ├── auth.py               # 认证路由
│   ├── chat.py               # 聊天路由
│   ├── contacts.py           # 联系人路由
│   ├── groups.py             # 群组路由
│   └── moments.py            # 朋友圈路由
├── tests/                     # 测试模块
│   ├── README.md             # 测试指南
│   ├── test_auth.py          # 认证测试
│   └── test_chat.py          # 聊天测试
├── scripts/                   # 脚本目录
│   └── init_test_data.py     # 测试数据初始化
└── docs/                      # 文档目录
    └── QUICKSTART.md         # 本文档
```

## 1. 安装依赖

```bash
cd backend
uv sync
```

## 2. 初始化测试数据

```bash
uv run python scripts/init_test_data.py
```

这会创建以下测试用户:
- alice
- bob
- charlie
- david
- eve

并添加一些好友关系。

## 3. 运行自动化测试

推荐使用 pytest 进行自动化测试（会自动启动/停止后端服务）：

```bash
# 运行所有测试
uv run pytest

# 运行特定测试
uv run pytest tests/test_auth.py -v

# 显示详细输出
uv run pytest -v -s
```

测试框架会自动：
- ✅ 启动后端服务器
- ✅ 初始化数据库
- ✅ 运行所有测试
- ✅ 停止服务器并清理

详见 [tests/README.md](../tests/README.md)

## 4. 手动启动服务器（可选）

如果需要手动测试或开发，可以启动服务器：

```bash
uv run python main.py
```

服务器将在 `http://localhost:8000` 启动。

## 5. 测试二维码登录

### 方式一：自动化测试（推荐）

```bash
uv run pytest tests/test_auth.py::test_qr_login_flow -v -s
```

这个测试会自动模拟用户扫码确认登录，无需手动操作。

### 方式二：手动测试

1. 启动服务器：`uv run python main.py`
2. 使用任何 WebSocket 客户端连接到 `ws://localhost:8000/ws`
3. 发送 `qr_login_init` 请求
4. 在浏览器中打开返回的 `qr_url`
5. 输入用户名（如 `alice`）并确认
6. 客户端会收到 `qr_confirmed` 推送

## 6. 在客户端中使用

C++ 客户端需要:

1. **生成二维码**: 使用 zxing 库将 `qr_url` 转换为二维码图片
2. **WebSocket 连接**: 使用 Qt WebSocket 连接到 `ws://localhost:8000/ws`
3. **发送/接收 JSON**: 所有消息都是 JSON 格式
4. **Token 持久化**: 将 token 保存到本地文件，下次启动时使用 `verify_token` 验证

## API 规范

所有 API 定义请参考 [../api-spec/](../../api-spec/) 目录：

- [common.md](../../api-spec/common.md) - 通用消息格式
- [auth.md](../../api-spec/auth.md) - 认证 API
- [chat.md](../../api-spec/chat.md) - 聊天 API
- [contacts.md](../../api-spec/contacts.md) - 联系人 API
- [groups.md](../../api-spec/groups.md) - 群组 API
- [moments.md](../../api-spec/moments.md) - 朋友圈 API

## 故障排除

### 端口被占用

如果 8000 端口被占用，修改 `main.py` 最后一行:

```python
uvicorn.run(app, host="0.0.0.0", port=8001)  # 改为其他端口
```

同时需要修改测试配置 `conftest.py` 中的端口。

### 数据库损坏

删除数据库文件重新初始化:

```bash
rm wetalk.db
uv run python scripts/init_test_data.py
```

### WebSocket 连接失败

确保:
1. 服务器正在运行（或使用 pytest 自动启动）
2. 防火墙没有阻止 8000 端口
3. 使用 `ws://` 而不是 `wss://`（本地开发不需要 SSL）

### 测试失败

如果测试失败，检查：
1. 数据库中是否有测试数据（运行 `scripts/init_test_data.py`）
2. 端口 8000 是否被占用
3. 查看详细错误信息：`uv run pytest -v -s`

## 开发工作流

### 日常开发

```bash
# 1. 运行测试确保一切正常
uv run pytest

# 2. 启动服务器进行手动测试
uv run python main.py

# 3. 修改代码后重新运行测试
uv run pytest -v
```

### 添加新功能

1. 在 `routers/` 中添加新的路由处理函数
2. 在 `main.py` 中注册新的消息类型
3. 在 `tests/` 中添加测试用例
4. 运行测试验证功能

### 调试

```bash
# 显示详细日志
uv run pytest -v -s

# 只运行失败的测试
uv run pytest --lf

# 进入调试模式
uv run pytest --pdb
```
