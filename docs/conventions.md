# 项目规范

## 目录结构规范

### 模块布局

每个模块位于 `src/<module>/`，所有 `.h` 和 `.cpp` 文件放在同一目录下，不分 `include/` 和 `src/` 子目录。

```
src/<module>/
├── CMakeLists.txt          # 模块构建配置
├── foo.h                   # 头文件（与 .cpp 同目录）
├── foo.cpp                 # 实现文件
├── bar.h
├── bar.cpp
├── tests/                  # 单元测试（GTest）
│   ├── test_foo.cpp
│   └── test_bar.cpp
└── sandbox/                # 可视化测试 GUI（仅有 GUI 的模块）
    └── main.cpp
```

### 头文件导出规则

- **模块内部头文件**：直接放在 `src/<module>/` 目录中，通过 `PRIVATE` include 引用
- **需要跨模块使用的导出头文件**：放到项目根目录的 `include/wechat/<module>/` 中，通过 `PUBLIC` include 引用
- 引用方式：`#include <wechat/core/types.h>`（导出头文件）或 `#include "foo.h"`（模块内部头文件）

```
include/
└── wechat/
    ├── core/               # core 模块导出的头文件
    │   └── types.h
    ├── chat/
    │   └── message.h
    └── ...
```

### 测试规范

- **单元测试**：放在各模块内部的 `tests/` 子目录，使用 GTest，编译为 `test_<module>` 可执行文件
- **Sandbox GUI**：放在各模块内部的 `sandbox/` 子目录，编译为 `sandbox_<module>` 可执行文件
- Sandbox 用于交互式可视化测试（如添加聊天消息、查看联系人列表等），不适合用单元测试覆盖的场景
- 通过 CMake 的 `ENABLE_TESTING=ON` 选项启用测试和 sandbox 编译

### 模块列表

| 模块 | 库名 | 说明 | 有 Sandbox |
|------|------|------|-----------|
| log | wechat_log | 日志模块（封装 spdlog） | 是 |
| core | wechat_core | 基础类型、工具函数 | 否 |
| storage | wechat_storage | SQLite3 本地存储 | 否 |
| network | wechat_network | gRPC 接口 + Mock | 否 |
| auth | wechat_auth | 登录、注册、会话管理 | 是 |
| chat | wechat_chat | 聊天消息 | 是 |
| contacts | wechat_contacts | 联系人管理 | 是 |
| moments | wechat_moments | 朋友圈/动态 | 是 |

### 模块依赖关系

```
log（spdlog PUBLIC）
core ← storage
core ← network
core + storage + network ← auth
core + storage + network + Qt ← chat
core + storage + network + Qt ← contacts
core + storage + network + Qt ← moments
```

## 代码规范

整体遵循 LLVM 编码风格。

### 文件命名

- 头文件：`.h`，源文件：`.cpp`
- 文件名使用 `PascalCase`：`ChatMessage.h`、`ChatMessage.cpp`
- 测试文件以 `Test` 为后缀：`ChatMessageTest.cpp`

### 命名约定

#### 类型名（class / struct / enum / typedef / using）

PascalCase：

```cpp
class ChatMessage;
struct UserInfo;
enum class MessageType;
using MessageList = std::vector<ChatMessage>;
```

#### 函数名 / 方法名

camelCase：

```cpp
void sendMessage();
int getUnreadCount();
bool isOnline() const;
```

#### 变量名

camelCase，**不使用任何前缀**（不用 `m_`、`_` 等）：

```cpp
class ChatMessage {
    std::string content;        // 成员变量，无前缀
    int64_t timestamp;
    MessageType type;

public:
    void setContent(std::string content) {
        this->content = std::move(content);  // 有歧义时显式 this->
    }

    std::string getContent() const {
        return content;                      // 无歧义时直接使用
    }
};
```

局部变量同样 camelCase：

```cpp
void processMessages() {
    int messageCount = 0;
    auto lastMessage = getLatestMessage();
}
```

#### 常量 / 枚举值

PascalCase：

```cpp
constexpr int MaxRetryCount = 3;
constexpr auto DefaultTimeout = std::chrono::seconds(30);

enum class MessageType {
    Text,
    Image,
    Voice,
    Video,
    File
};
```

#### 命名空间

小写，与模块名一致：

```cpp
namespace wechat::core {
    // ...
}

namespace wechat::chat {
    // ...
}
```

#### 宏（尽量避免使用）

全大写 + 下划线：

```cpp
#define WECHAT_VERSION_MAJOR 1
```

### this 指针使用规则

- 成员变量**不加任何前缀**
- 无歧义时直接使用成员变量名
- 参数名与成员变量名冲突时，使用 `this->` 消歧义

```cpp
// 正确
class User {
    std::string name;
    int age;

    void setName(std::string name) {
        this->name = std::move(name);   // 参数同名，用 this->
    }

    void printInfo() const {
        spdlog::info("name={}, age={}", name, age);  // 无歧义，直接用
    }
};

// 错误 - 不要这样做
class User {
    std::string m_name;     // ✗ 不用 m_ 前缀
    std::string _name;      // ✗ 不用 _ 前缀
    std::string name_;      // ✗ 不用 _ 后缀
};
```

### 头文件保护

使用 `#pragma once`，不使用传统 include guard：

```cpp
#pragma once

#include <string>
// ...
```

### include 顺序

1. 对应的头文件（仅 `.cpp` 文件）
2. 项目导出头文件 `<wechat/...>`
3. 模块内部头文件 `"..."`
4. 第三方库头文件
5. 标准库头文件

每组之间空一行：

```cpp
// ChatMessage.cpp
#include "ChatMessage.h"

#include <wechat/core/types.h>

#include "MessageFormatter.h"

#include <spdlog/spdlog.h>
#include <QObject>

#include <string>
#include <vector>
```



### CMake 目标命名

| 类别 | 命名 | 示例 |
|------|------|------|
| 库目标 | `wechat_<module>` | `wechat_core` |
| 单元测试 | `test_<module>` | `test_chat` |
| Sandbox | `sandbox_<module>` | `sandbox_chat` |
| 导出头文件路径 | `include/wechat/<module>/` | `include/wechat/core/` |

### CMake 链接规则

所有依赖统一使用 `PUBLIC` 链接，包括第三方库和项目内部模块：

```cmake
target_link_libraries(wechat_foo
    PUBLIC
        wechat_core
        wechat_network
        wechat_log
        Qt6::Core
)
```
