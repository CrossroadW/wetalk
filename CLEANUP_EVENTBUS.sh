#!/bin/bash
# 清理 EventBus 重构后的文件
# 在项目根目录运行: bash CLEANUP_EVENTBUS.sh

echo "Cleaning up EventBus files..."

# 删除 EventBus.h
rm -f "include/wechat/core/EventBus.h"
echo "✓ Deleted include/wechat/core/EventBus.h"

# 删除 Event.h
rm -f "include/wechat/core/Event.h"
echo "✓ Deleted include/wechat/core/Event.h"

# 删除 EventBus.cpp（如果存在）
rm -f "src/core/EventBus.cpp"
echo "✓ Deleted src/core/EventBus.cpp (if existed)"

echo ""
echo "Cleanup complete!"
echo "You can now rebuild the project:"
echo "  cd build && cmake .. && cmake --build . -j4"
