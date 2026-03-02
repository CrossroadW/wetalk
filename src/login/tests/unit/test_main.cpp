#include <gtest/gtest.h>
#include <QCoreApplication>

int main(int argc, char** argv) {
    // 初始化 QCoreApplication（所有测试共享）
    QCoreApplication app(argc, argv);

    // 初始化 Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // 运行所有测试
    return RUN_ALL_TESTS();
}
