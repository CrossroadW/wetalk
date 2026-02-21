#pragma once

#include <wechat/network/MomentService.h>

#include <memory>
namespace wechat::network {

class MockDataStore;

class MockMomentService : public MomentService {
public:
    explicit MockMomentService(std::shared_ptr<MockDataStore> store);

    Result<Moment> postMoment(
        const std::string& token, const std::string& text,
        const std::vector<std::string>& imageIds) override;
    Result<std::vector<Moment>> listMoments(
        const std::string& token, int64_t beforeTs, int limit) override;
    VoidResult likeMoment(
        const std::string& token, int64_t momentId) override;
    Result<Moment::Comment> commentMoment(
        const std::string& token, int64_t momentId,
        const std::string& text) override;

private:
    std::shared_ptr<MockDataStore> store;
};

} // namespace wechat::network
