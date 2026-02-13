#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>

namespace wechat {
namespace storage {

class FriendshipDao {
public:
    explicit FriendshipDao(SQLite::Database& db);

    /// 添加好友（自动排序 a < b，幂等）
    void add(const std::string& userA, const std::string& userB);
    /// 删除好友
    void remove(const std::string& userA, const std::string& userB);
    /// 是否为好友
    bool isFriend(const std::string& userA, const std::string& userB);
    /// 获取某用户的所有好友 id
    std::vector<std::string> findFriends(const std::string& userId);

private:
    /// 保证 a < b，消除方向性
    static std::pair<std::string, std::string> ordered(
        const std::string& a, const std::string& b);
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
