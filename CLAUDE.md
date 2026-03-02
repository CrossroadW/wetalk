# WeTalk - 项目指南

> 基于 C++23 / Qt6 的微信客户端克隆项目。此文件是 Claude Code 的入口索引。

## 文档

- `README.md` — 项目介绍、开发路线图、技术栈、环境要求
- `TODO.md` — 各模块待办事项和完成进度
- `docs/conventions.md` — 编码规范（命名、目录结构、CMake、include 顺序等）
- `docs/constraints.md` — 项目约束（CMake、Qt、Pimpl、命名空间、模块化）
- `docs/data-models.md` — 数据模型定义、数据库表结构、类型映射
- `docs/data-cache-mechanism.md` — 客户端增量缓存机制设计（两个数据库 + 两个同步）
- `docs/plan-cache-refactor.md` — 缓存同步重构方案（当前阶段实施计划）
- `docs/websocket-client-design.md` — WebSocket 客户端实现设计（二维码登录 + 实时通信）

## API 规范

- `api-spec/` — **前后端通信 API 规范（独立目录）**
- `api-spec/README.md` — 规范总览和索引
- `api-spec/common.md` — 通用消息格式、数据类型定义
- `api-spec/auth.md` — 认证 API（二维码登录、Token 验证）
- `api-spec/chat.md` — 聊天 API（消息发送、获取、编辑、撤回）
- `api-spec/contacts.md` — 联系人 API（好友列表、添加好友）
- `api-spec/groups.md` — 群组 API（群组列表、创建群组）
- `api-spec/moments.md` — 朋友圈 API（获取朋友圈、创建朋友圈）
- `api-spec/errors.md` — 错误处理规范
- `api-spec/flows.md` — 认证流程、实现要求

## 后端

- `backend/` — FastAPI + WebSocket + SQLite3 后端服务
- `backend/README.md` — 后端文档（引用 api-spec）
- `backend/QUICKSTART.md` — 后端快速开始指南
- `backend/main.py` — WebSocket 服务器实现
- `backend/database.py` — SQLite 数据库初始化
- `backend/init_test_data.py` — 测试数据初始化脚本
- `backend/test_websocket.py` — WebSocket API 测试脚本

## 关键目录

- `include/wechat/<module>/` — 跨模块导出的公共头文件
- `src/<module>/` — 模块实现，每个模块含 `tests/` 和可选的 `sandbox/`
- `docs/` — 项目文档
- `conan/` — Conan 配置（debug/release profiles）

## 关键文件

- `include/wechat/core/Message.h` — 消息数据结构（variant 内容块，支持图文混排）
- `include/wechat/core/User.h` — 用户数据结构（id, username, password, token）
- `include/wechat/core/Group.h` — 群组数据结构
- `include/wechat/chat/ChatPresenter.h` — 聊天 MVP Presenter（消息同步 + 信号）
- `include/wechat/chat/SessionPresenter.h` — 会话列表 Presenter（群组列表 + 实时更新）
- `include/wechat/login/LoginPresenter.h` — 登录 MVP Presenter（登录/注册 + 信号）
- `include/wechat/contacts/ContactsPresenter.h` — 通讯录 Presenter（好友管理 + 搜索）
- `include/wechat/network/NetworkClient.h` — 网络客户端抽象工厂
- `include/wechat/network/ChatService.h` — 聊天服务接口（发送/同步/撤回/编辑 + 推送通知信号）
- `include/wechat/network/AuthService.h` — 认证服务接口（注册/登录/登出）
- `include/wechat/network/ContactService.h` — 联系人服务接口
- `include/wechat/network/GroupService.h` — 群组服务接口
- `include/wechat/network/MomentService.h` — 朋友圈服务接口
- `src/chat/ChatWidget.h` — 聊天消息界面（MVP View）
- `src/chat/ChatPage.h` — 聊天页面（SessionListWidget + ChatWidget 组合）
- `src/chat/SessionListWidget.h` — 会话列表界面
- `src/chat/MockBackend.h` — 模拟后端（预灌数据 + 定时脚本测试）
- `src/login/LoginWidget.h` — 登录/注册界面
- `src/contacts/ContactsWidget.h` — 通讯录界面（好友列表 + 搜索）
- `src/network/MockDataStore.h` — SQLite 内存数据存储（Mock 后端）
- `src/main.cpp` — 主应用入口（Login → MainWindow with ChatPage + ContactsWidget）

## 架构

```
┌──────────────────────────────────────────────┐
│  main.cpp (MainWindow)                       │
│  ┌────────┬─────────────────────────────────┐│
│  │ TabBar │ Content (QStackedWidget)        ││
│  │        │                                 ││
│  │ [Chat] │ ChatPage (SessionList + Chat)   ││
│  │ [Cont] │ ContactsWidget                  ││
│  │        │                                 ││
│  └────────┴─────────────────────────────────┘│
└──────────────────────────────────────────────┘
        ▲ loginSuccess 信号触发跳转
┌──────────────────────────────────────────────┐
│  LoginWidget (登录/注册界面)                   │
└──────────────────────────────────────────────┘
```

ChatPage 内部：
```
┌──────────────┬────────────────────────┐
│ SessionList  │  ChatWidget            │
│              │                        │
│ Alice        │  [MessageListView]     │
│ Group1       │                        │
│              │  [Input + Send]        │
└──────────────┴────────────────────────┘
```

### MVP 数据流

```
UI 层 (Qt Widgets)       LoginWidget, ChatWidget, ContactsWidget, SessionListWidget
    ↕ Qt signals/slots
Presenter (QObject)      LoginPresenter, ChatPresenter, SessionPresenter, ContactsPresenter
    ↕ 方法调用
服务层 (QObject)         NetworkClient → WebSocketClient → ChatService, AuthService, ContactService, GroupService
    ↕ WebSocket (JSON)
后端服务                 FastAPI + WebSocket + SQLite3
```

**二维码登录流程**:
1. LoginWidget → LoginPresenter.startQRLogin()
2. WebSocketClient 发送 `qr_login_init` → 后端返回 session_id + qr_url
3. LoginWidget 使用 zxing 生成二维码显示 qr_url
4. 用户扫码 → 浏览器打开网页 → 输入用户名（无需密码）
5. 后端验证用户名 → 生成 token → 推送 `qr_confirmed` 给 WebSocketClient
6. LoginPresenter 收到推送 → Q_EMIT loginSuccess(User, token)
7. MainWindow 保存 token 到本地文件

**消息发送**:
ChatWidget → ChatPresenter.sendMessage() → ChatService → WebSocketClient 发送 `send_message` → 后端存储 → 返回消息 → Q_EMIT messageStored → ChatPresenter.fetchAfter → Q_EMIT messagesInserted → ChatWidget

**消息接收**:
后端推送 `new_message` → WebSocketClient → ChatService Q_EMIT messageStored → ChatPresenter.fetchAfter → Q_EMIT messagesInserted → ChatWidget

**撤回/编辑**:
ChatPresenter → ChatService → WebSocketClient 发送 `revoke_message`/`edit_message` → 后端更新 → 返回成功 → Q_EMIT messageUpdated → ChatWidget

## 依赖与工具

spdlog 1.17.0, gtest 1.17.0, sqlitecpp 3.3.3, boost 1.86.0 (UUID), Qt6 (Core/Widgets/Network/WebSockets), zxing-cpp (二维码生成)

C++23 / CMake 3.24+ / Conan 2.0+ / MSVC / Ninja Multi-Config / `ENABLE_TESTING=ON`

后端: Python 3.11+ / FastAPI / uvicorn / websockets / uv (包管理)
