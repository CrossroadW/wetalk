#pragma once

#include "wechat/core/Group.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wechat {
namespace storage {

class GroupDao {
public:
    explicit GroupDao(SQLite::Database& db);

    // ── groups_ 表 ──
    void insertGroup(const core::Group& group, int64_t now);
    void updateOwner(const std::string& groupId, const std::string& ownerId, int64_t now);
    void removeGroup(const std::string& groupId);
    std::optional<core::Group> findGroupById(const std::string& id);

    // ── group_members 表 ──
    void addMember(const std::string& groupId, const std::string& userId, int64_t now);
    void removeMember(const std::string& groupId, const std::string& userId, int64_t now);
    std::vector<std::string> findMemberIds(const std::string& groupId);
    std::vector<std::string> findGroupIdsByUser(const std::string& userId);

    // ── 增量同步 ──
    std::vector<core::Group> findGroupsUpdatedAfter(int64_t since);

    /// 成员变更增量同步：返回 {group_id, user_id, removed} 三元组
    struct MemberChange {
        std::string groupId;
        std::string userId;
        bool removed;
        int64_t updatedAt;
    };
    std::vector<MemberChange> findMemberChangesAfter(int64_t since);

private:
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
