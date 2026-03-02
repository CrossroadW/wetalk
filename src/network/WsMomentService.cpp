#include "WsMomentService.h"

namespace wechat {
namespace network {

WsMomentService::WsMomentService(WebSocketClient& ws)
    : ws(ws) {}

Result<Moment> WsMomentService::postMoment(
    const std::string& token,
    const std::string& text,
    const std::vector<std::string>& imageIds) {
    return std::unexpected("Not implemented yet");
}

Result<std::vector<Moment>> WsMomentService::listMoments(
    const std::string& token,
    int64_t beforeTs,
    int limit) {
    return std::unexpected("Not implemented yet");
}

VoidResult WsMomentService::likeMoment(
    const std::string& token,
    int64_t momentId) {
    return std::unexpected("Not implemented yet");
}

Result<Moment::Comment> WsMomentService::commentMoment(
    const std::string& token,
    int64_t momentId,
    const std::string& text) {
    return std::unexpected("Not implemented yet");
}

} // namespace network
} // namespace wechat
