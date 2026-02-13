# Project Overview - CLAUDE.md

> 此文件记录项目的结构和依赖信息，用于快速了解项目现状

## 📋 最后更新时间
2025-01-XX

## 📁 项目结构

```
./
├── conan/                    # Conan 配置目录
│   ├── debug/
│   │   └── profile          # Debug 配置文件
│   └── release/
│       └── profile          # Release 配置文件
├── include/                  # 头文件目录
│   └── wechat/
│       ├── core/            # 核心模块（User, Message, EventBus 等）
│       ├── log/             # 日志模块
│       ├── auth/            # 认证模块
│       ├── chat/            # 聊天模块
│       ├── contacts/        # 联系人模块
│       ├── moments/         # 朋友圈模块
│       ├── network/         # 网络模块
│       └── storage/         # 存储模块
├── src/                      # 源代码目录
│   ├── main.cpp             # 主程序入口
│   ├── core/                # 核心模块实现
│   ├── log/                 # 日志模块实现
│   ├── auth/                # 认证模块实现
│   ├── chat/                # 聊天模块实现
│   ├── contacts/            # 联系人模块实现
│   ├── moments/             # 朋友圈模块实现
│   ├── network/             # 网络模块实现
│   └── storage/             # 存储模块实现
├── build/                    # 构建输出目录（自动生成）
├── .vscode/
│   └── tasks.json           # VSCode 任务配置
├── CMakeLists.txt           # CMake 构建配置
├── conanfile.py             # Conan 依赖配置
├── .gitignore               # Git 忽略文件
├── README.md                # 项目说明
└── CLAUDE.md                # 此文件
```

## 📦 项目依赖

通过 `conanfile.py` 统一管理：

- **spdlog**: 1.17.0 - 高性能日志库
- **gtest**: 1.17.0 - Google 测试框架
- **boost**: 1.90.0 - Boost 库（headers）
- **sqlitecpp**: 3.3.3 - SQLite C++ 封装
- **Qt6**: Core, Widgets, Network - GUI 框架（系统安装）

## 🔧 构建工具版本

- CMake: >= 3.24
- Conan: >= 2.0
- Generator: Ninja Multi-Config
- C++ Standard: C++23

## 🚀 快速命令

| 任务 | 命令 |
|-----|------|
| 安装依赖 | `conan install . --build=missing` |
| 配置项目 | `conan install . --build=missing` (自动生成 CMake 配置) |
| 构建 Debug | `cmake --build build --config Debug` |
| 构建 Release | `cmake --build build --config Release` |
| 运行 Debug | `.\build\Debug\wetalk.exe` |
| 运行 Release | `.\build\Release\wetalk.exe` |
| 清理缓存 | `conan cache clean "*"` |
| 查看缓存 | `conan list "*"` |

## 🧹 Conan 缓存管理

| 操作 | 命令 |
|-----|------|
| 查看缓存位置 | `conan cache path` |
| 查看所有包 | `conan list "*"` |
| 清理特定包 | `conan remove "包名/*" -c` |
| 清理所有包 | `conan remove "*" -c` |
| 清理构建缓存 | `conan cache clean "*"` |
| 清理源码和构建 | `conan cache clean "*" --source --build` |

**缓存位置**: `C:\Users\<用户名>\.conan2\p`

## 📝 重要文件说明

| 文件 | 说明 |
|-----|------|
| `conanfile.py` | Conan 依赖配置（统一管理所有依赖） |
| `conan/debug/profile` | Debug 编译配置文件 |
| `conan/release/profile` | Release 编译配置文件 |
| `CMakeLists.txt` | CMake 构建规则定义 |
| `.vscode/tasks.json` | VSCode 集成任务 |

## 🏗️ 模块架构

项目采用模块化设计，每个模块包含：
- 头文件：`include/wechat/<模块>/`
- 实现文件：`src/<模块>/`
- 单元测试：`src/<模块>/tests/`
- 沙盒测试：`src/<模块>/sandbox/`
- CMake 配置：`src/<模块>/CMakeLists.txt`

**核心模块**：
- `core` - 核心数据结构和事件总线
- `log` - 日志系统
- `storage` - 数据持久化
- `network` - 网络通信
- `auth` - 用户认证
- `chat` - 聊天功能
- `contacts` - 联系人管理
- `moments` - 朋友圈功能

## 📌 当前编译配置

- **C++ 标准**: C++23
- **编译器**: MSVC (Windows) / GCC / Clang
- **Build Types**: Debug, Release (Ninja Multi-Config)
- **平台**: Windows (可跨平台)
- **GUI 框架**: Qt6

## ⚙️ 环境检查清单

- [ ] 已安装 Conan 2.0+
- [ ] 已安装 CMake 3.24+
- [ ] 已安装 Ninja
- [ ] 已安装 C++23 编译器 (MSVC 2022 / gcc 11+ / clang 14+)
- [ ] 已安装 Qt6 (Core, Widgets, Network)
- [ ] 运行过 `conan install . --build=missing`

## 🔍 常见问题

**Q: 如何清理 Conan 缓存？**
A: 使用 `conan cache clean "*"` 清理构建缓存，或 `conan remove "*" -c` 删除所有包

**Q: 如何重新构建项目？**
A: 删除 `build` 目录，然后重新运行 `conan install . --build=missing`

**Q: 如何添加新的依赖？**
A: 在 `conanfile.py` 的 `requirements()` 方法中添加，然后重新运行 `conan install`

---

**提示**: 当项目结构或依赖有重大变化时，请及时更新此文件。
