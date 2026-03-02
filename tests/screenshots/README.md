# 界面截图测试框架

## 概述

本测试框架用于自动化验证 UI 界面效果，通过截图对比来确保界面符合设计预期。

## 目录结构

```
tests/screenshots/
├── login/
│   ├── expected/          # 预期效果图（提交到 git）
│   │   ├── README.md      # 状态说明文档
│   │   ├── 01_qr_code_initial.png
│   │   ├── 02_qr_code_loading.png
│   │   ├── 03_qr_code_scanned.png
│   │   └── 04_direct_login.png
│   └── output/            # 测试生成的截图（git 忽略）
│       └── *.png
├── chat/
│   ├── expected/
│   └── output/
└── contacts/
    ├── expected/
    └── output/
```

## 工作原理

### 1. 预期图片（expected/）

- 手工放置的微信参考截图
- 提交到 git，作为测试基准
- 每个图片对应一个特定的 UI 状态
- 配有 README.md 说明如何进入该状态

### 2. 生成图片（output/）

- 测试代码自动生成的截图
- 被 git 忽略，不提交
- 用于与 expected/ 中的图片对比

### 3. 测试代码

每个模块的 `tests/*_screen_test.cpp` 文件：
- 使用 QtTest 框架
- 模拟各种 UI 状态
- 自动截图保存到 output/
- 使用 PROJECT_ROOT_PATH 宏定位输出目录

**命名约定**：
- 单元测试：`test_*.cpp`（如 `test_login.cpp`）
- 截图测试：`*_screen_test.cpp`（如 `login_screen_test.cpp`）

## 使用方法

### 运行测试

```bash
# 构建测试
cmake --build build --target test_login_screen

# 运行测试
./build/src/login/test_login_screen
```

### 查看结果

测试完成后，检查 `tests/screenshots/login/output/` 目录：
- 对比生成的图片与 expected/ 中的预期图片
- 可以使用图片对比工具或人工检查

### 更新预期图片

如果界面设计有意更改：

```bash
# 1. 运行测试生成新截图
./build/src/login/test_login_screen

# 2. 人工确认新截图符合设计要求

# 3. 复制到 expected/
cp tests/screenshots/login/output/*.png tests/screenshots/login/expected/

# 4. 提交到 git
git add tests/screenshots/login/expected/
git commit -m "Update login UI screenshots"
```

## 编写新的截图测试

### 1. 创建目录结构

```bash
mkdir -p tests/screenshots/your_module/{expected,output}
```

### 2. 添加预期图片

将微信参考截图放到 `expected/` 目录，并创建 README.md 说明。

### 3. 编写测试代码

参考 `src/login/tests/login_screen_test.cpp`：

```cpp
#include <QtTest/QtTest>
#include <QApplication>
#include <QDir>

class YourModuleScreenTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
        // 使用 QDir 构建路径，避免字符串拼接
        QDir rootDir(PROJECT_ROOT_PATH);
        QString outputPath = rootDir.filePath("tests");
        outputPath = QDir(outputPath).filePath("screenshots");
        outputPath = QDir(outputPath).filePath("your_module");
        outputPath = QDir(outputPath).filePath("output");

        QDir outputDir(outputPath);
        if (!outputDir.exists()) {
            outputDir.mkpath(".");
        }
        QDir::setCurrent(outputDir.absolutePath());
    }

    void test01_some_state() {
        // 设置 UI 状态
        // ...

        saveScreenshot("01_some_state.png");
    }

private:
    void saveScreenshot(const QString& filename) {
        ensureWidgetShown();
        QTest::qWait(100);
        QPixmap pix = m_widget->grab();
        QImage img = pix.toImage();
        img.save(filename, "PNG", 70);  // 70% 质量
    }
};

#include "your_module_screen_test.moc"
```

**文件命名**：`your_module_screen_test.cpp`（小写，下划线分隔）

### 4. 更新 CMakeLists.txt

在模块的 CMakeLists.txt 中添加：

```cmake
if(ENABLE_TESTING)
    # 单元测试 (test_*.cpp)
    file(GLOB MODULE_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)
    if(MODULE_UNIT_TEST_SOURCES)
        add_executable(test_module ${MODULE_UNIT_TEST_SOURCES})
        target_compile_definitions(test_module PRIVATE QT_NO_KEYWORDS)
        target_link_libraries(test_module PUBLIC wechat_module
                GTest::gtest_main Qt6::Test)
        gtest_discover_tests(test_module)
    endif()

    # 截图测试 (*_screen_test.cpp)
    file(GLOB MODULE_SCREEN_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_screen_test.cpp)
    if(MODULE_SCREEN_TEST_SOURCES)
        qt_add_executable(test_module_screen ${MODULE_SCREEN_TEST_SOURCES})
        target_compile_definitions(test_module_screen PRIVATE
            QT_NO_KEYWORDS
            PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}")
        set_target_properties(test_module_screen PROPERTIES AUTOMOC ON)
        target_link_libraries(test_module_screen PUBLIC
            wechat_module Qt6::Test Qt6::Widgets)
    endif()
endif()
```

## 最佳实践

### 路径处理

**禁止字符串拼接路径**：
```cpp
// ❌ 错误：字符串拼接
QString path = QString(PROJECT_ROOT_PATH) + "/tests/screenshots/login/output";

// ✅ 正确：使用 QDir
QDir rootDir(PROJECT_ROOT_PATH);
QString path = rootDir.filePath("tests");
path = QDir(path).filePath("screenshots");
path = QDir(path).filePath("login");
path = QDir(path).filePath("output");
```

使用 `QDir::filePath()` 的优势：
- 自动处理不同操作系统的路径分隔符
- 避免路径格式错误
- 代码更清晰、更安全

### 截图质量

- 使用固定窗口尺寸（如 800x600）
- 降低 PNG 质量到 70% 节省存储
- 避免包含动画或时间戳

### 状态命名

- 使用数字前缀排序：`01_`, `02_`, `03_`
- 使用描述性名称：`qr_code_initial`, `chat_message_sent`
- 保持一致的命名风格

### 文档说明

每个 expected/ 目录必须包含 README.md，说明：
- 每个截图代表的状态
- 如何进入该状态
- 预期的视觉效果

### Git 管理

- ✅ 提交：expected/ 中的预期图片和 README.md
- ❌ 忽略：output/ 中的生成图片
- ✅ 提交：测试代码 *ScreenTest.cpp

## 注意事项

1. **不要自动对比** - 当前框架只生成截图，不自动对比
2. **人工验证** - 需要人工检查生成的截图是否符合预期
3. **跨平台差异** - 不同操作系统的渲染可能有细微差异
4. **分辨率** - 使用较低分辨率节省 token 和存储空间

## 未来改进

- [ ] 自动像素对比
- [ ] 感知哈希算法对比
- [ ] CI/CD 集成
- [ ] 差异高亮显示
- [ ] 批量对比工具
