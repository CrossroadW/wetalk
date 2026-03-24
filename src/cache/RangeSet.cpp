#include "RangeSet.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace wechat {
namespace cache {

void RangeSet::addRange(int32_t start, int32_t end) {
    if (start > end) return;

    ranges_.push_back({start, end});
    std::sort(ranges_.begin(), ranges_.end(),
              [](const Range& a, const Range& b) { return a.start < b.start; });

    std::vector<Range> merged;
    for (const auto& r : ranges_) {
        if (merged.empty() || r.start > merged.back().end + 1) {
            merged.push_back(r);
        } else {
            merged.back().end = std::max(merged.back().end, r.end);
        }
    }
    ranges_ = std::move(merged);
}

bool RangeSet::contains(int32_t chatSeq) const {
    for (const auto& r : ranges_) {
        if (chatSeq >= r.start && chatSeq <= r.end) return true;
        if (r.start > chatSeq) break; // 已升序，可提前退出
    }
    return false;
}

int32_t RangeSet::firstSeq() const {
    if (ranges_.empty()) return 0;
    return ranges_.front().start;
}

int32_t RangeSet::lastSeq() const {
    if (ranges_.empty()) return 0;
    return ranges_.back().end;
}

} // namespace cache
} // namespace wechat
