#pragma once

namespace wechat {
namespace cache {

/// 消息插入方向提示，供 UI 层决定滚动策略
enum class InsertHint {
    Top,     // 前插（加载历史）→ 保持锚点，防跳动
    Bottom,  // 后追加（新消息/触底）→ 在底部则滚到底
    Replace, // 清空重填（jumpTo）→ 滚到指定 chatSeq
};

} // namespace cache
} // namespace wechat
