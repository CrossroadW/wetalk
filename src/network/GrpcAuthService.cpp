#include "GrpcAuthService.h"
#include <wechat/network/NetworkTypes.h>

// #include "auth.grpc.pb.h"
// #include "auth.pb.h"

// #include <grpcpp/support/status.h>

namespace wechat {
namespace network {

// namespace proto = wechat::auth;

GrpcAuthService::GrpcAuthService(const std::string& serverAddress)
    : channel(nullptr) {
    // TODO: 当后端就绪时启用 gRPC
    // channel = grpc::CreateChannel(serverAddress,
    //                               grpc::InsecureChannelCredentials());
}

Result<RegisterResponse> GrpcAuthService::registerUser(
    const std::string& username, const std::string& password) {
    // TODO: 实现真实的 gRPC 调用
    return {ErrorCode::Unavailable,
            "gRPC backend not available yet - use MockNetworkClient"};
}

Result<LoginResponse> GrpcAuthService::login(
    const std::string& username, const std::string& password) {
    return {ErrorCode::Unavailable,
            "gRPC backend not available yet - use MockNetworkClient"};
}

VoidResult GrpcAuthService::logout(const std::string& token) {
    return {ErrorCode::Unavailable,
            "gRPC backend not available yet - use MockNetworkClient"};
}

Result<core::User> GrpcAuthService::getCurrentUser(const std::string& token) {
    return {ErrorCode::Unavailable,
            "gRPC backend not available yet - use MockNetworkClient"};
}

} // namespace network
} // namespace wechat
