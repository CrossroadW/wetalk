# 项目约束规范

## CMake 依赖管理规范

### 基本原则
- 所有的 `find_package` 调用必须在根目录的 CMakeLists.txt 中进行
- 模块内的 CMakeLists.txt 文件禁止包含 `find_package` 调用
- 模块内的 CMakeLists.txt 文件禁止包含 `cmake_minimum_required` 和 `project` 声明
- 所有 MOC、RCC、UIC 等 Qt 工具必须针对特定目标使用，不能全局应用

### 适用范围
此规范适用于以下模块：
- src/core/
- src/log/
- src/storage/
- src/network/
- src/auth/
- src/chat/
- src/contacts/
- src/moments/
- src/grpc/

### 理由
1. **集中管理**：所有依赖在一个地方定义，便于维护
2. **版本一致性**：确保所有模块使用相同版本的依赖库
3. **减少重复**：避免在每个子模块中重复查找相同的包
4. **构建性能**：减少重复的依赖查找过程
5. **Qt 工具明确性**：确保 Qt 元对象编译器等工具应用于正确的可执行文件或库

### Qt 工具使用规范
- 使用 `qt_add_executable()` 而不是 `add_executable()` 来自动处理 MOC、RCC、UIC
- 如果必须手动指定，应使用 `qt_wrap_cpp()`、`qt_add_resources()`、`qt_add_ui()` 等针对特定目标
- 避免全局的 Qt 工具调用，以确保构建的可预测性和可维护性

### 例外情况
- 测试和沙盒目标的特殊配置可在模块内 CMakeLists.txt 中定义（如 Qt Widgets 连接）
- 模块特定的编译选项和定义可以在模块 CMakeLists.txt 中定义