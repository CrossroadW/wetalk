# EventBus 重构总结

## 概述

完成了全局 EventBus 的重构，使用 **信号注入模式** 替代全局总线。这符合第一性原理的设计思想：模块间通信仅需两个信号（发/收），使用 Boost.Signals2 实现信号类。

## 核心变更

### 1. 新增 ChatSignals 类
**文件**: `include/wechat/chat/ChatSignals.h`

纯信号聚合类，包含 5 个 Boost.Signals2 信号：
- `messageSent` - 消息发送成功
- `messageSendFailed` - 消息发送失败
- `messagesReceived` - 接收新消息
- `messageRevoked` - 消息撤回
- `messageEdited` - 消息编辑

所有信号参数都是公共数据模型（纯数据，无状态）。

### 2. ChatManager 修改
**文件**: `include/wechat/chat/ChatManager.h` | `src/chat/ChatManager.cpp`

**变更**：
- 构造函数：`ChatManager(NetworkClient&, EventBus&)` → `ChatManager(NetworkClient&, shared_ptr<ChatSignals>)`
- 删除：`EventBus& bus_` 成员
- 替换为：`shared_ptr<ChatSignals> signals_` 成员
- 所有 `bus_.publish()` 替换为 `signals_->signal()` 直接触发

**示例**：
```cpp
// 改前
bus_.publish(core::MessageSentEvent{tempId, std::move(msg)});

// 改后
signals_->messageSent(tempId, msg);
```

### 3. ChatController 修改
**文件**: `src/chat/ChatController.h` | `src/chat/ChatController.cpp`

**变更**：
- 构造函数：`ChatController(ChatManager&, EventBus&, QObject*)` → `ChatController(ChatManager&, shared_ptr<ChatSignals>, QObject*)`
- 删除：`busConnection_` 单一连接
- 替换为：5 个独立的 `scoped_connection`（每个信号一个）
- 删除：`onEvent()` 方法（不再需要 variant 分发）
- 新增：lambda 订阅逻辑，直接在构造函数中处理信号

**示例**：
```cpp
// 改前：单一 onEvent 方法处理所有事件
busConnection_ = bus_.subscribe([this](const Event& e) { onEvent(e); });

// 改后：直接订阅信号
messageSentConnection_ = signals_->messageSent.connect(
    [this](const std::string& clientTempId, const Message& msg) {
        emit messageSent(QString::fromStdString(clientTempId), msg);
    });
```

### 4. 沙箱/示例代码更新
**文件**: `src/chat/sandbox/main.cpp`

**变更**：
- 删除：`core::EventBus bus;`
- 新增：`auto signals = std::make_shared<chat::ChatSignals>();`
- 传参：`ChatManager(networkClient, signals)` 和 `ChatController(manager, signals)`

### 5. 测试更新
**文件**: `src/core/tests/test_core.cpp`

**变更**：
- 删除：所有 EventBus 单元测试（7 个 TEST 函数）
- 保留：SQLite 相关测试

## 待清理文件

以下文件已不再被使用，需要删除：
1. `include/wechat/core/EventBus.h` - 全局事件总线头文件
2. `include/wechat/core/Event.h` - 事件 variant 定义
3. `src/core/EventBus.cpp` - 事件总线实现（如果存在）

**清理方式**：运行项目根目录的清理脚本：
```bash
bash CLEANUP_EVENTBUS.sh
```

或手动删除这三个文件。

## 架构改进

### 改前
```
所有模块 → 全局 EventBus (性能低，耦合度高)
```

### 改后
```
Chat模块 + 网络层 + 其他模块
    ↕ ChatSignals（注入）
主程序（组织者）
    ↕
各模块之间无直接依赖，仅通过信号通信
```

## 编译与验证

### 重新编译
```bash
cd build
cmake ..
cmake --build . -j4
```

### 运行测试
```bash
ctest --output-on-failure
```

### 检查依赖
所有的 `#include <wechat/core/EventBus.h>` 和 `#include <wechat/core/Event.h>` 都已移除。

运行此命令验证没有遗留引用：
```bash
grep -r "EventBus.h\|Event\.h" include/ src/ --include="*.h" --include="*.cpp" || echo "No references found"
```

## 后续步骤

### 1. 实现其他模块的信号类
以相同的模式为其他模块创建信号类：
- `NetworkToChatSignals` - 网络层通知 Chat 层
- `ChatToNetworkSignals` - Chat 层请求网络层
- `ContactSignals` - 联系人相关信号
- 等等

### 2. 更新主程序 (main.cpp)
在 `src/main.cpp` 中组织所有信号注入：
```cpp
auto networkToChat = std::make_shared<NetworkToChatSignals>();
auto chatToNetwork = std::make_shared<ChatToNetworkSignals>();

Network network(chatToNetwork);
Chat chat(networkToChat, chatToNetwork);
// ...
```

### 3. 模块完全解耦
确保模块间仅通过信号通信，不存在直接的头文件依赖（除了数据模型）。

## 参考

- Boost.Signals2 文档: https://www.boost.org/doc/libs/1_90_0/doc/html/signals2.html
- 项目约束: `docs/constraints.md`
- 项目规范: `docs/conventions.md`
