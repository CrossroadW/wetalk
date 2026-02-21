# 项目约束规范

> 所有约束集中在此文件，覆盖 CMake、Qt、代码风格、模块化等方面

## CMake 约束

### 依赖管理
- 所有 `find_package` 调用必须在根目录 `CMakeLists.txt` 中
- 模块 CMakeLists.txt 禁止包含 `find_package`、`cmake_minimum_required`、`project`
- 所有依赖统一使用 `PUBLIC` 链接

### Qt 工具
- MOC、RCC、UIC 必须针对特定目标设置，不能全局应用
- 使用 `qt_add_executable()` 而非 `add_executable()`
- 不为不需要的目录添加不必要的 AUTOMOC 等设置

### 适用范围
src/core、src/log、src/storage、src/network、src/auth、src/chat、src/contacts、src/moments

## Qt 使用约束

- **非界面代码禁止使用 Qt 信号槽**（core、network、storage 等纯 C++ 模块）
- Qt 信号槽仅用于 UI 层（Widget、View）和 Presenter 层（如 ChatPresenter）
- 模块间通信使用 Boost.Signals2 信号注入
- core、network、storage、auth 等非 UI 模块不依赖 Qt 信号槽
- Presenter 层（如 ChatPresenter）作为 QObject，订阅 Boost.Signals2 网络通知并转为 Qt signals 供 View 使用

## Pimpl 约束

- `include/` 目录下导出的公共头文件必须使用 Pimpl 模式
- `src/` 内部的实现类无需使用 Pimpl

## 命名空间约束

### .cpp 文件
- 允许使用 `using namespace` 简化代码
- 推荐在文件开头使用，如 `using namespace wechat;`

### .h 文件
- **严格禁止** 任何形式的 `using namespace`
- 所有类型必须使用完整命名空间限定符
- 命名空间声明使用非嵌套形式：

```cpp
// 正确
namespace wechat {
namespace core {
}
}

// 错误
namespace wechat::core {
}
```

## 模块化约束

- 单个源文件长度控制在 500-800 行以内
- 按功能职责拆分代码到不同模块
- 头文件只声明，不包含不必要的实现

## UI 组件约束

- 优先使用标准 Qt 控件，避免复杂自绘
- 标准控件提供原生交互体验（文本选择、复制等）
