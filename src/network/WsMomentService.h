#pragma once

#include <wechat/network/MomentService.h>
#include <wechat/network/NetworkTypes.h>
#include <wechat/network/WebSocketClient.h>

namespace wechat {
namespace network {

class WsMomentService : public MomentService {
public:
    explicit WsMomentService(WebSocketClient& ws);

    Result<Moment> postMoment(
        const std::string& token,
        const std::string& text,
        const std::vector<std::string>& imageIds) override;

    Result<std::vector<Moment>> listMoments(
        const std::string& token,
        int64_t beforeTs,
        int limit) override;

    VoidResult likeMoment(
        const std::string& token,
        int64_t momentId) override;

    Result<Moment::Comment> commentMoment(
        const std::string& token,
        int64_t momentId,
        const std::string& text) override;

private:
    WebSocketClient& ws;
};

} // namespace network
} // namespace wechat
