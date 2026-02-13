# WeChat Clone

基于 C++23 / Qt6 的微信客户端克隆项目。

## 开发路线图

### 阶段 1：客户端（毕业设计）

> 目标：完成微信核心功能的客户端实现，所有业务逻辑和测试均在客户端完成。

- **本地存储**：使用 SQLite3 存储聊天记录等本地数据
- **后端存储（预规划）**：后端计划使用 SQLite3 / RocksDB 等，阶段 1 先按此设计 Mock
- **通信协议**：客户端基于 gRPC 设计接口，为后续对接后端做好准备
- **后端模拟**：不实现真实后端，通过 Mock 层模拟分布式后端的行为，按照最终架构设计进行开发
- **测试策略**：
  - 单元测试：覆盖各模块核心逻辑
  - 集成测试：模拟网络交互，验证端到端流程
- **完成标志**：微信核心功能可用后进入下一阶段

### 阶段 2：后端原型（功能验证）

> 目标：使用 Node.js / Python 快速搭建分布式后端原型，验证架构可行性。

- 实现 gRPC 服务端，与阶段 1 客户端对接
- 搭建分布式架构 + 负载均衡的最小可用版本
- 不追求性能，优先跑通核心链路

### 阶段 3：后端正式版（高性能）

> 目标：使用 Go / Rust / C++ 重写后端，实现生产级的高性能分布式系统。

- 替换阶段 2 的原型后端
- 完整的分布式部署、负载均衡、容灾方案
- 性能优化与压力测试

## 模块结构

每个模块自包含：源码、测试、sandbox 均在模块目录内。详见 [docs/conventions.md](docs/conventions.md)。

```
src/<module>/
├── *.h *.cpp               # 源文件（同目录，不分 include/src）
├── tests/                  # 单元测试（GTest）
└── sandbox/                # 可视化测试 GUI

include/wechat/<module>/    # 仅存放需要跨模块导出的头文件
```

| 模块 | 说明 | Sandbox |
|------|------|---------|
| core | 基础类型、工具函数 | - |
| storage | SQLite3 本地存储 | - |
| network | gRPC 接口 + Mock | - |
| auth | 登录、注册、会话管理 | sandbox_auth |
| chat | 聊天消息 | sandbox_chat |
| contacts | 联系人管理 | sandbox_contacts |
| moments | 朋友圈/动态 | sandbox_moments |

## 技术栈

| 类别 | 选型 |
|------|------|
| 语言 | C++23 |
| GUI | Qt 6.5 |
| 日志 | spdlog |
| 构建 | CMake 3.24+ / Visual Studio 17 2022 |
| 包管理 | Conan 2.0+ |
| 测试 | GTest 1.14 |
| 通信 | gRPC（计划中） |
| 本地存储 | SQLite3 / RocksDB（计划中） |

## 环境要求

- Visual Studio 2022（MSVC 工具链）
- CMake 3.24+
- Conan 2.0+
- Qt 6.5.3（默认路径 `C:/Qt/6.5.3/msvc2019_64`）

## 许可证

MIT
