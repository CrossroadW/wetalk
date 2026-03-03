# WeTalk Backend

基于 FastAPI + WebSocket + SQLite3 的微信后端服务。

## 📋 API 规范

**重要**: 前后端开发必须严格遵守 [../api-spec/](../api-spec/) 中定义的规范。

所有消息格式、字段定义、错误处理都在规范中明确定义：

- [README.md](../api-spec/README.md) - 总览和索引
- [common.md](../api-spec/common.md) - 通用消息格式、数据类型
- [auth.md](../api-spec/auth.md) - 认证 API（二维码登录、Token 验证）
- [chat.md](../api-spec/chat.md) - 聊天 API（消息发送、获取、编辑、撤回）
- [contacts.md](../api-spec/contacts.md) - 联系人 API（好友管理）
- [groups.md](../api-spec/groups.md) - 群组 API（群组管理）
- [moments.md](../api-spec/moments.md) - 朋友圈 API（朋友圈管理）
- [errors.md](../api-spec/errors.md) - 错误处理规范
- [flows.md](../api-spec/flows.md) - 认证流程说明

## 特性

- **WebSocket 通信**: 所有客户端通信使用 WebSocket，支持实时推送
- **二维码登录**: 仅需用户名，无需密码（适合演示）
- **Token 认证**: 支持 token 持久化，记住登录状态
- **完整功能**: 聊天、联系人、群组、朋友圈
- **模块化设计**: 清晰的目录结构，易于维护和扩展

## 项目结构

```
backend/
├── main.py                    # 应用入口
├── core/                      # 核心模块（数据库）
├── managers/                  # 管理器模块（连接管理）
├── routers/                   # 路由处理模块（业务逻辑）
├── tests/                     # 测试模块
├── scripts/                   # 脚本目录
└── docs/                      # 文档目录
```

## 快速开始

### 安装依赖

使用 uv 管理依赖：

```bash
cd backend
uv sync
```

### 初始化测试数据

```bash
uv run python scripts/init_test_data.py
```

### 运行服务器

```bash
uv run python main.py
```

服务器将在 `http://localhost:8000` 启动。

### 运行测试

```bash
# 运行所有测试
uv run pytest

# 运行特定测试
uv run pytest tests/test_auth.py -v
```

详见 [tests/README.md](tests/README.md) 和 [docs/QUICKSTART.md](docs/QUICKSTART.md)

## 二维码登录流程

1. **桌面客户端**: 发送 `qr_login_init` 请求
2. **服务器**: 返回 `session_id` 和 `qr_url`
3. **桌面客户端**: 使用 zxing 生成二维码显示 `qr_url`
4. **手机用户**: 扫描二维码，打开网页
5. **网页**: 显示登录表单，用户输入用户名
6. **网页**: POST 到 `/api/qr-confirm`
7. **服务器**: 验证用户名，生成 token，推送 `qr_confirmed` 给桌面客户端
8. **桌面客户端**: 收到推送，保存 token，进入主界面

下次启动时，客户端使用保存的 token 发送 `verify_token` 请求，无需重新登录。

## 数据库

SQLite3 数据库文件: `wetalk.db`

### 表结构

- `users` - 用户表（id, username, password, token）
- `groups_` - 群组表（id, owner_id, updated_at）
- `group_members` - 群组成员（group_id, user_id, joined_at, removed, updated_at）
- `friendships` - 好友关系（user_id_a, user_id_b）
- `messages` - 消息表（id, sender_id, chat_id, reply_to, content_data, revoked, read_count, updated_at）
- `moments` - 朋友圈（id, author_id, text, timestamp, updated_at）
- `moment_images` - 朋友圈图片（moment_id, image_id, sort_order）
- `moment_likes` - 朋友圈点赞（moment_id, user_id）
- `moment_comments` - 朋友圈评论（id, moment_id, author_id, text, timestamp）
- `qr_sessions` - 二维码登录会话（session_id, status, user_id, created_at, expires_at）

### 索引

- `idx_group_members_user` - 群组成员用户索引
- `idx_messages_chat` - 消息聊天索引
- `idx_messages_reply` - 消息回复索引
- `idx_messages_updated` - 消息更新时间索引
- `idx_moments_author` - 朋友圈作者索引
- `idx_moment_comments_moment` - 朋友圈评论索引
- `idx_qr_sessions_expires` - 二维码会话过期索引

## 开发

### 添加测试用户

运行初始化脚本：

```bash
uv run python scripts/init_test_data.py
```

或直接操作数据库：

```bash
sqlite3 wetalk.db
```

```sql
INSERT INTO users (username, password) VALUES ('alice', '');
INSERT INTO users (username, password) VALUES ('bob', '');
```

注意: password 字段保留但不使用，可以为空字符串。

### 清空数据库

```bash
rm wetalk.db
uv run python main.py  # 会自动重新创建
```

### 运行测试

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

## 技术栈

- **FastAPI** - 现代 Python Web 框架
- **WebSocket** - 实时双向通信
- **SQLite3** - 轻量级数据库
- **Uvicorn** - ASGI 服务器
- **pytest** - 测试框架
- **httpx** - 异步 HTTP 客户端（测试用）
