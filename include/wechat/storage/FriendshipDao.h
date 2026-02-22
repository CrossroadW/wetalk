#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <vector>

namespace wechat {
namespace storage {

class FriendshipDao {
public:
    explicit FriendshipDao(SQLite::Database& db);

    /// 添加好友（自动排序 a < b，幂等）
    void add(int64_t userA, int64_t userB);
    /// 删除好友
    void remove(int64_t userA, int64_t userB);
    /// 是否为好友
    bool isFriend(int64_t userA, int64_t userB);
    /// 获取某用户的所有好友 id
    std::vector<int64_t> findFriends(int64_t userId);

private:
    /// 保证 a < b，消除方向性
    static std::pair<int64_t, int64_t> ordered(int64_t a, int64_t b);
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
