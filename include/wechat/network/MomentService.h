#pragma once

#include <wechat/network/NetworkTypes.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat::network {

/// 朋友圈动态
struct Moment {
    int64_t id = 0;
    std::string authorId;
    std::string text;
    std::vector<std::string> imageIds;
    int64_t timestamp;
    std::vector<std::string> likedBy;

    struct Comment {
        int64_t id = 0;
        std::string authorId;
        std::string text;
        int64_t timestamp;
    };
    std::vector<Comment> comments;
};

/// 朋友圈服务接口
class MomentService {
public:
    virtual ~MomentService() = default;

    /// 发布动态
    virtual Result<Moment> postMoment(
        const std::string& token,
        const std::string& text,
        const std::vector<std::string>& imageIds) = 0;

    /// 获取朋友圈列表（分页）
    virtual Result<std::vector<Moment>> listMoments(
        const std::string& token,
        int64_t beforeTs,
        int limit) = 0;

    /// 点赞
    virtual VoidResult likeMoment(
        const std::string& token,
        int64_t momentId) = 0;

    /// 评论
    virtual Result<Moment::Comment> commentMoment(
        const std::string& token,
        int64_t momentId,
        const std::string& text) = 0;
};

} // namespace wechat::network
