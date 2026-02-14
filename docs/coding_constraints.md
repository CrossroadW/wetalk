# 代码约束规范

## 命名空间使用规范

### 实现文件 (.cpp)
- 允许使用 `using namespace` 声明来简化代码
- 推荐在文件开头使用，例如：`using namespace wechat;`
- 这样可以避免冗长的命名空间前缀，提高代码可读性

### 头文件 (.h/.hpp)
- **严格禁止** 使用任何形式的 `using namespace` 声明
- 所有类型必须使用完整命名空间限定符
- 例如：`wechat::core::Message` 而不是 `Message`
- 这是为了防止污染包含该头文件的其他文件的命名空间

### 示例对比

**在 .cpp 文件中（允许）：**
```cpp
#include <some_header.h>
using namespace wechat;

int main() {
    core::Message msg;  // 可以直接使用 core:: 前缀
    return 0;
}
```

**在 .h 文件中（禁止）：**
```cpp
#pragma once
#include <other_header.h>

using namespace wechat;  // ❌ 禁止！

namespace my_namespace {
    // 一些代码
}
```

**在 .h 文件中（正确做法）：**
```cpp
#pragma once
#include <other_header.h>

namespace my_namespace {
    wechat::core::Message msg;  // ✅ 使用完整命名空间
}
```

## 目的
1. 在实现文件中提高开发效率和代码可读性
2. 在头文件中防止命名空间污染，确保库的干净接口
3. 保持代码模块化，提升可维护性
4. 减少不必要的配置，保持构建系统简洁
5. 使用标准控件实现UI功能，避免复杂的自绘实现

## 模块化规范

### 文件拆分原则
- 单个源文件长度应控制在合理范围内（通常不超过500-800行）
- 按功能职责拆分代码到不同的模块
- 避免单一文件承担过多职责
- 头文件只需声明，不应包含不必要的实现细节

### UI组件实现规范
- 优先使用标准Qt控件实现UI功能
- 避免复杂的自绘实现，除非有特殊视觉需求
- 标准控件能够提供原生的交互体验（如文本选择、复制等）

### CMakeLists.txt 配置
- 遵循最小配置原则
- 不为不需要的目录（如测试目录）添加不必要的 AUTOMOC 等设置
- 使用通配符时注意排除不需要的目录（tests, sandbox 等）