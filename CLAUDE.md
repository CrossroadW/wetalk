# Project Overview - CLAUDE.md

> 此文件记录项目的结构和依赖信息，用于快速了解项目现状

## 📋 最后更新时间
2026-02-13 10:30

## 📁 项目结构

```
./
├── conan/                    # Conan 配置目录
│   ├── debug/
│   │   └── conanfile.txt    # Debug 配置
│   └── release/
│       └── conanfile.txt    # Release 配置
├── src/
│   └── main.cpp             # 主程序源文件
├── .vscode/
│   └── tasks.json           # VSCode 任务配置
├── CMakeLists.txt           # CMake 构建配置
├── CMakePresets.json        # CMake 预设配置
├── .gitignore               # Git 忽略文件
├── README.md                # 项目说明
└── CLAUDE.md                # 此文件
```

## 📦 项目依赖

### Debug 配置
- **fmt**: 10.1.1
- **build_type**: Debug
- **compiler**: gcc 11

### Release 配置
- **fmt**: 10.1.1
- **build_type**: Release
- **compiler**: gcc 11

## 🔧 构建工具版本

- CMake: >= 3.24
- Conan: >= 2.0
- Generator: Ninja
- C++ Standard: C++17

## 🚀 快速命令

| 任务 | 命令 |
|-----|------|
| 安装依赖 | `Ctrl+Shift+B` 或 `conan install conan/debug conan/release` |
| Debug 构建 | `cmake --preset debug && cmake --build --preset debug` |
| Release 构建 | `cmake --preset release && cmake --build --preset release` |
| 运行 Debug | `./build/debug/myapp` |
| 运行 Release | `./build/release/myapp` |

## 📝 重要文件说明

| 文件 | 说明 |
|-----|------|
| `conan/debug/conanfile.txt` | Debug 编译的依赖配置 |
| `conan/release/conanfile.txt` | Release 编译的依赖配置 |
| `CMakeLists.txt` | CMake 构建规则定义 |
| `CMakePresets.json` | Debug/Release 预设配置 |
| `.vscode/tasks.json` | VSCode 集成任务 |

## 📌 当前编译配置

- **C++ 标准**: C++17
- **编译器**: gcc 11 (需要根据实际环境修改)
- **Build Types**: Debug, Release
- **平台**: Windows (可跨平台)

## ⚙️ 环境检查清单

- [ ] 已安装 Conan 2.0+
- [ ] 已安装 CMake 3.24+
- [ ] 已安装 Ninja
- [ ] 已安装 C++17 编译器 (gcc/clang/MSVC)
- [ ] 运行过 `Ctrl+Shift+B` 安装依赖

---

**提示**: 当项目结构或依赖有重大变化时，请运行 VSCode 任务更新此文件。
