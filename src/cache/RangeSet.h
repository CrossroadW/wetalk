#pragma once

#include <cstdint>
#include <vector>

namespace wechat {
namespace cache {

/// per-chat 已加载的 chatSeq 区间集合，自动合并重叠/相邻区间。
///
/// chatSeq 是从 1 开始的 per-chat 密集自增序号，无跨 chat 间隙，
/// 因此区间 [start, end] 内的 chatSeq 对该聊天均已加载。
class RangeSet {
public:
    struct Range {
        int32_t start;
        int32_t end;
    };

    /// 添加区间，自动合并重叠/相邻区间
    void addRange(int32_t start, int32_t end);

    /// chatSeq 是否在已加载区间内
    bool contains(int32_t chatSeq) const;

    bool isEmpty() const { return ranges_.empty(); }

    /// 所有区间中最小的 start（用于触顶加载游标）
    int32_t firstSeq() const;

    /// 所有区间中最大的 end（用于触底加载游标）
    int32_t lastSeq() const;

    void clear() { ranges_.clear(); }

    const std::vector<Range>& ranges() const { return ranges_; }

private:
    std::vector<Range> ranges_; // 保持升序排列，无重叠
};

} // namespace cache
} // namespace wechat
