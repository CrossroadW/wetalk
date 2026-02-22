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
服务层 (QObject)         NetworkClient → ChatService, AuthService, ContactService, GroupService
    ↕
数据层                   MockDataStore (SQLite :memory:)
```

发送: ChatWidget → ChatPresenter.sendMessage() → ChatService → Q_EMIT messageStored → fetchAfter → Q_EMIT messagesInserted → ChatWidget

接收: ChatService Q_EMIT messageStored → ChatPresenter.fetchAfter → Q_EMIT messagesInserted → ChatWidget

撤回/编辑: ChatPresenter → ChatService → Q_EMIT messageUpdated → fetchMessage → Q_EMIT messageUpdated → ChatWidget

登录: LoginWidget → LoginPresenter.login() → AuthService → Q_EMIT loginSuccess(User) → MainWindow

## 依赖与工具

spdlog 1.17.0, gtest 1.17.0, sqlitecpp 3.3.3, boost 1.86.0 (UUID), Qt6 (Core/Widgets/Network)

C++23 / CMake 3.24+ / Conan 2.0+ / MSVC / Ninja Multi-Config / `ENABLE_TESTING=ON`
