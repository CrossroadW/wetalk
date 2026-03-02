# WeTalk API 规范

> 前后端通信的完整 API 规范，前后端必须严格遵守。

## 版本

**当前版本**: v1.0 (2026-03-02)

## 连接信息

- **协议**: WebSocket
- **端点**: `ws://localhost:8000/ws`
- **消息格式**: JSON

## 文档结构

本规范按模块拆分为多个文件，便于查阅和 AI 使用：

### 基础规范
- [common.md](common.md) - 通用消息格式、数据类型定义、连接信息

### API 模块
- [auth.md](auth.md) - 认证相关 API（二维码登录、Token 验证、登出）
- [chat.md](chat.md) - 聊天相关 API（消息发送、获取、编辑、撤回）
- [contacts.md](contacts.md) - 联系人相关 API（好友列表、添加好友）
- [groups.md](groups.md) - 群组相关 API（群组列表、创建群组）
- [moments.md](moments.md) - 朋友圈相关 API（获取朋友圈、创建朋友圈）
- [test.md](test.md) - 测试相关 API（数据库重置，仅测试环境）

### 其他
- [errors.md](errors.md) - 错误处理规范、常见错误信息
- [flows.md](flows.md) - 认证流程、实现要求

## 快速开始

### 后端开发

```bash
# 告诉 AI
"请根据 api-spec/auth.md 实现认证功能"
"请根据 api-spec/chat.md 实现聊天功能"
```

### 前端开发

```bash
# 告诉 AI
"请根据 api-spec/common.md 和 api-spec/auth.md 实现 C++ WebSocket 客户端的认证模块"
"请根据 api-spec/chat.md 实现聊天消息发送和接收"
```

### 验证一致性

```bash
cd backend
uv run python test_websocket.py  # 测试后端是否符合规范
```

## 使用原则

1. **API First**: 先定义规范，再实现代码
2. **严格遵守**: 前后端必须完全按照规范实现
3. **版本管理**: 规范变更时更新版本号
4. **文档优先**: 有疑问时以规范文档为准

## 版本历史

- **v1.0** (2026-03-02): 初始版本
  - 定义基本消息格式
  - 实现认证、聊天、联系人、群组、朋友圈 API
  - 支持二维码登录和 token 验证
