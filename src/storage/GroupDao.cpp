#include "wechat/storage/GroupDao.h"

namespace wechat {
namespace storage {

GroupDao::GroupDao(SQLite::Database& db) : db_(db) {}

// ── groups_ 表 ──

void GroupDao::insertGroup(const core::Group& group, int64_t now) {
    SQLite::Statement stmt(db_,
        "INSERT OR REPLACE INTO groups_ (id, owner_id, updated_at) VALUES (?, ?, ?)");
    stmt.bind(1, group.id);
    stmt.bind(2, group.ownerId);
    stmt.bind(3, now);
    stmt.exec();

    // 同时插入成员
    for (const auto& uid : group.memberIds) {
        addMember(group.id, uid, now);
    }
}

void GroupDao::updateOwner(const std::string& groupId,
                           const std::string& ownerId, int64_t now) {
    SQLite::Statement stmt(db_,
        "UPDATE groups_ SET owner_id = ?, updated_at = ? WHERE id = ?");
    stmt.bind(1, ownerId);
    stmt.bind(2, now);
    stmt.bind(3, groupId);
    stmt.exec();
}

void GroupDao::removeGroup(const std::string& groupId) {
    db_.exec("DELETE FROM group_members WHERE group_id = '" + groupId + "'");
    db_.exec("DELETE FROM groups_ WHERE id = '" + groupId + "'");
}

std::optional<core::Group> GroupDao::findGroupById(const std::string& id) {
    SQLite::Statement stmt(db_,
        "SELECT id, owner_id FROM groups_ WHERE id = ?");
    stmt.bind(1, id);
    if (!stmt.executeStep()) return std::nullopt;

    core::Group g;
    g.id = stmt.getColumn(0).getString();
    g.ownerId = stmt.getColumn(1).getString();
    g.memberIds = findMemberIds(id);
    return g;
}

// ── group_members 表 ──

void GroupDao::addMember(const std::string& groupId,
                         const std::string& userId, int64_t now) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO group_members (group_id, user_id, joined_at, removed, updated_at)
        VALUES (?, ?, ?, 0, ?)
        ON CONFLICT(group_id, user_id) DO UPDATE
            SET removed = 0, updated_at = excluded.updated_at
    )");
    stmt.bind(1, groupId);
    stmt.bind(2, userId);
    stmt.bind(3, now);
    stmt.bind(4, now);
    stmt.exec();
}

void GroupDao::removeMember(const std::string& groupId,
                            const std::string& userId, int64_t now) {
    SQLite::Statement stmt(db_, R"(
        UPDATE group_members SET removed = 1, updated_at = ?
        WHERE group_id = ? AND user_id = ?
    )");
    stmt.bind(1, now);
    stmt.bind(2, groupId);
    stmt.bind(3, userId);
    stmt.exec();
}

std::vector<std::string> GroupDao::findMemberIds(const std::string& groupId) {
    std::vector<std::string> ids;
    SQLite::Statement stmt(db_,
        "SELECT user_id FROM group_members WHERE group_id = ? AND removed = 0");
    stmt.bind(1, groupId);
    while (stmt.executeStep()) {
        ids.push_back(stmt.getColumn(0).getString());
    }
    return ids;
}

std::vector<std::string> GroupDao::findGroupIdsByUser(const std::string& userId) {
    std::vector<std::string> ids;
    SQLite::Statement stmt(db_,
        "SELECT group_id FROM group_members WHERE user_id = ? AND removed = 0");
    stmt.bind(1, userId);
    while (stmt.executeStep()) {
        ids.push_back(stmt.getColumn(0).getString());
    }
    return ids;
}

// ── 增量同步 ──

std::vector<core::Group> GroupDao::findGroupsUpdatedAfter(int64_t since) {
    std::vector<core::Group> result;
    SQLite::Statement stmt(db_,
        "SELECT id, owner_id FROM groups_ WHERE updated_at > ?");
    stmt.bind(1, since);
    while (stmt.executeStep()) {
        core::Group g;
        g.id = stmt.getColumn(0).getString();
        g.ownerId = stmt.getColumn(1).getString();
        g.memberIds = findMemberIds(g.id);
        result.push_back(std::move(g));
    }
    return result;
}

std::vector<GroupDao::MemberChange> GroupDao::findMemberChangesAfter(int64_t since) {
    std::vector<MemberChange> result;
    SQLite::Statement stmt(db_, R"(
        SELECT group_id, user_id, removed, updated_at
        FROM group_members WHERE updated_at > ?
        ORDER BY updated_at ASC
    )");
    stmt.bind(1, since);
    while (stmt.executeStep()) {
        result.push_back({
            stmt.getColumn(0).getString(),
            stmt.getColumn(1).getString(),
            stmt.getColumn(2).getInt() != 0,
            stmt.getColumn(3).getInt64()
        });
    }
    return result;
}

} // namespace storage
} // namespace wechat
