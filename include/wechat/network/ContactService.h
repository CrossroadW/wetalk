#pragma once

#include <wechat/core/User.h>
#include <wechat/network/NetworkTypes.h>

#include <string>
#include <vector>

namespace wechat::network {

/// 联系人服务接口
class ContactService {
public:
    virtual ~ContactService() = default;

    /// 添加好友
    virtual VoidResult addFriend(
        const std::string& token,
        const std::string& targetUserId) = 0;

    /// 删除好友
    virtual VoidResult removeFriend(
        const std::string& token,
        const std::string& targetUserId) = 0;

    /// 获取好友列表
    virtual Result<std::vector<core::User>> listFriends(
        const std::string& token) = 0;

    /// 按关键字搜索用户
    virtual Result<std::vector<core::User>> searchUser(
        const std::string& token,
        const std::string& keyword) = 0;
};

} // namespace wechat::network
