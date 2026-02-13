# Implementation Constraints

> 实现层面的约束和规范

## Qt 使用约束

- **非界面相关的代码禁止使用 Qt 信号槽机制**
- Qt 信号槽仅用于 UI 层（Widget、View 等可见组件）之间的通信
- 模块间通信、事件系统、业务逻辑层必须使用纯 C++ 实现
- core、network、storage、auth 等非 UI 模块不依赖 Qt 信号槽

## Pimpl 约束

- **`include/` 目录下导出的公共头文件必须使用 Pimpl 模式**
- `src/` 内部的实现类无需使用 Pimpl