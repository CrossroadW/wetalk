#pragma once

#include "wechat/core/Group.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace wechat {
namespace storage {

class GroupDao {
public:
    explicit GroupDao(SQLite::Database& db);

    // ── groups_ 表 ──
    void insertGroup(const core::Group& group, int64_t now);
    void updateOwner(int64_t groupId, int64_t ownerId, int64_t now);
    void removeGroup(int64_t groupId);
    std::optional<core::Group> findGroupById(int64_t id);

    // ── group_members 表 ──
    void addMember(int64_t groupId, int64_t userId, int64_t now);
    void removeMember(int64_t groupId, int64_t userId, int64_t now);
    std::vector<int64_t> findMemberIds(int64_t groupId);
    std::vector<int64_t> findGroupIdsByUser(int64_t userId);

    // ── 增量同步 ──
    std::vector<core::Group> findGroupsUpdatedAfter(int64_t since);

    /// 成员变更增量同步：返回 {group_id, user_id, removed} 三元组
    struct MemberChange {
        int64_t groupId;
        int64_t userId;
        bool removed;
        int64_t updatedAt;
    };
    std::vector<MemberChange> findMemberChangesAfter(int64_t since);

private:
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
