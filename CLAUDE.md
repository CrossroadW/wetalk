# WeTalk - 项目指南

> 基于 C++23 / Qt6 的微信客户端克隆项目。此文件是 Claude Code 的入口索引。

## 文档

- `README.md` — 项目介绍、开发路线图、技术栈、环境要求
- `TODO.md` — 各模块待办事项和完成进度
- `docs/conventions.md` — 编码规范（命名、目录结构、CMake、include 顺序等）
- `docs/constraints.md` — 项目约束（CMake、Qt、Pimpl、命名空间、模块化）
- `docs/data-models.md` — 数据模型定义、数据库表结构、类型映射
- `docs/data-cache-mechanism.md` — 客户端增量缓存机制设计

## 关键目录

- `include/wechat/<module>/` — 跨模块导出的公共头文件
- `src/<module>/` — 模块实现，每个模块含 `tests/` 和可选的 `sandbox/`
- `docs/` — 项目文档
- `conan/` — Conan 配置（debug/release profiles）

## 关键文件

- `include/wechat/core/Event.h` — 事件类型定义（MessageSent/SendFailed/Received/Revoked/Edited）
- `include/wechat/core/EventBus.h` — 发布-订阅事件总线（Boost.Signals2）
- `include/wechat/core/Message.h` — 消息数据结构（variant 内容块，支持图文混排）
- `include/wechat/chat/ChatManager.h` — 聊天业务逻辑（纯 C++，不依赖 Qt）
- `include/wechat/network/NetworkClient.h` — 网络客户端抽象工厂
- `include/wechat/network/ChatService.h` — 聊天服务接口（发送/同步/撤回/编辑）
- `src/chat/ChatController.h` — Qt 桥接层（EventBus 事件 → Qt signals + QTimer 轮询）
- `src/chat/ChatWidget.h` — 主聊天界面
- `src/chat/MockAutoResponder.h` — 模拟对方用户发消息
- `src/network/MockDataStore.h` — 内存数据存储（Mock 后端）

## 架构

```
UI 层 (Qt Widgets)       ChatWidget, MessageListView, MessageItemWidget
    ↕ Qt signals/slots
桥接层 (QObject)         ChatController — EventBus 事件转 Qt signals + QTimer 轮询
    ↕ EventBus
业务层 (纯 C++)          ChatManager — 发送/同步/撤回/编辑
    ↕ 同步调用
服务层                   NetworkClient → ChatService, AuthService, ContactService...
    ↕
数据层                   MockDataStore (内存) / SQLite (计划中)
```

发送: ChatWidget → ChatController → ChatManager → ChatService.sendMessage() → EventBus(MessageSentEvent) → ChatController → ChatWidget

接收: QTimer → ChatManager.pollMessages() → syncMessages() → EventBus(MessagesReceivedEvent) → ChatController → ChatWidget

## 依赖与工具

spdlog 1.17.0, gtest 1.17.0, boost 1.90.0 (Signals2), sqlitecpp 3.3.3, Qt6 (Core/Widgets/Network)

C++23 / CMake 3.24+ / Conan 2.0+ / MSVC / Ninja Multi-Config / `ENABLE_TESTING=ON`
