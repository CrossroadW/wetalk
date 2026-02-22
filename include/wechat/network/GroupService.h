#pragma once

#include <wechat/core/Group.h>
#include <wechat/network/NetworkTypes.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat::network {

/// 群组服务接口
class GroupService {
public:
    virtual ~GroupService() = default;

    /// 创建群组，返回新群
    virtual Result<core::Group> createGroup(
        const std::string& token,
        const std::vector<int64_t>& memberIds) = 0;

    /// 解散群组（仅群主）
    virtual VoidResult dissolveGroup(
        const std::string& token,
        int64_t groupId) = 0;

    /// 添加成员
    virtual VoidResult addMember(
        const std::string& token,
        int64_t groupId,
        int64_t userId) = 0;

    /// 移除成员（仅群主）
    virtual VoidResult removeMember(
        const std::string& token,
        int64_t groupId,
        int64_t userId) = 0;

    /// 获取群成员列表
    virtual Result<std::vector<int64_t>> listMembers(
        const std::string& token,
        int64_t groupId) = 0;

    /// 获取我加入的所有群
    virtual Result<std::vector<core::Group>> listMyGroups(
        const std::string& token) = 0;
};

} // namespace wechat::network
