#pragma once

#include <grpcpp/grpcpp.h>
#include <wechat/network/AuthService.h>
#include <memory>
#include <string>

namespace wechat {
namespace network {

class GrpcAuthService : public AuthService {
public:
    explicit GrpcAuthService(const std::string& serverAddress);

    Result<RegisterResponse> registerUser(
        const std::string& username,
        const std::string& password) override;

    Result<LoginResponse> login(
        const std::string& username,
        const std::string& password) override;

    VoidResult logout(const std::string& token) override;

    Result<core::User> getCurrentUser(const std::string& token) override;

private:
    std::unique_ptr<grpc::Channel> channel;
};

} // namespace network
} // namespace wechat
