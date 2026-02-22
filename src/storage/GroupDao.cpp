#include "wechat/storage/GroupDao.h"

namespace wechat {
namespace storage {

GroupDao::GroupDao(SQLite::Database& db) : db_(db) {}

// ── groups_ 表 ──

int64_t GroupDao::insertGroup(const core::Group& group, int64_t now) {
    SQLite::Statement stmt(db_,
        "INSERT INTO groups_ (owner_id, updated_at) VALUES (?, ?)");
    stmt.bind(1, group.ownerId);
    stmt.bind(2, now);
    stmt.exec();
    auto id = db_.getLastInsertRowid();

    // 同时插入成员
    for (auto uid : group.memberIds) {
        addMember(id, uid, now);
    }
    return id;
}

void GroupDao::updateOwner(int64_t groupId, int64_t ownerId, int64_t now) {
    SQLite::Statement stmt(db_,
        "UPDATE groups_ SET owner_id = ?, updated_at = ? WHERE id = ?");
    stmt.bind(1, ownerId);
    stmt.bind(2, now);
    stmt.bind(3, groupId);
    stmt.exec();
}

void GroupDao::removeGroup(int64_t groupId) {
    SQLite::Statement stmt1(db_, "DELETE FROM group_members WHERE group_id = ?");
    stmt1.bind(1, groupId);
    stmt1.exec();
    SQLite::Statement stmt2(db_, "DELETE FROM groups_ WHERE id = ?");
    stmt2.bind(1, groupId);
    stmt2.exec();
}

std::optional<core::Group> GroupDao::findGroupById(int64_t id) {
    SQLite::Statement stmt(db_,
        "SELECT id, owner_id FROM groups_ WHERE id = ?");
    stmt.bind(1, id);
    if (!stmt.executeStep()) return std::nullopt;

    core::Group g;
    g.id = stmt.getColumn(0).getInt64();
    g.ownerId = stmt.getColumn(1).getInt64();
    g.memberIds = findMemberIds(id);
    return g;
}

// ── group_members 表 ──

void GroupDao::addMember(int64_t groupId, int64_t userId, int64_t now) {
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

void GroupDao::removeMember(int64_t groupId, int64_t userId, int64_t now) {
    SQLite::Statement stmt(db_, R"(
        UPDATE group_members SET removed = 1, updated_at = ?
        WHERE group_id = ? AND user_id = ?
    )");
    stmt.bind(1, now);
    stmt.bind(2, groupId);
    stmt.bind(3, userId);
    stmt.exec();
}

std::vector<int64_t> GroupDao::findMemberIds(int64_t groupId) {
    std::vector<int64_t> ids;
    SQLite::Statement stmt(db_,
        "SELECT user_id FROM group_members WHERE group_id = ? AND removed = 0");
    stmt.bind(1, groupId);
    while (stmt.executeStep()) {
        ids.push_back(stmt.getColumn(0).getInt64());
    }
    return ids;
}

std::vector<int64_t> GroupDao::findGroupIdsByUser(int64_t userId) {
    std::vector<int64_t> ids;
    SQLite::Statement stmt(db_,
        "SELECT group_id FROM group_members WHERE user_id = ? AND removed = 0");
    stmt.bind(1, userId);
    while (stmt.executeStep()) {
        ids.push_back(stmt.getColumn(0).getInt64());
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
        g.id = stmt.getColumn(0).getInt64();
        g.ownerId = stmt.getColumn(1).getInt64();
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
            stmt.getColumn(0).getInt64(),
            stmt.getColumn(1).getInt64(),
            stmt.getColumn(2).getInt() != 0,
            stmt.getColumn(3).getInt64()
        });
    }
    return result;
}

} // namespace storage
} // namespace wechat
