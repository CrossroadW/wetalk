#include "wechat/storage/FriendshipDao.h"

namespace wechat {
namespace storage {

FriendshipDao::FriendshipDao(SQLite::Database& db) : db_(db) {}

std::pair<int64_t, int64_t> FriendshipDao::ordered(int64_t a, int64_t b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

void FriendshipDao::add(int64_t userA, int64_t userB) {
    auto [a, b] = ordered(userA, userB);
    SQLite::Statement stmt(db_,
        "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)");
    stmt.bind(1, a);
    stmt.bind(2, b);
    stmt.exec();
}

void FriendshipDao::remove(int64_t userA, int64_t userB) {
    auto [a, b] = ordered(userA, userB);
    SQLite::Statement stmt(db_,
        "DELETE FROM friendships WHERE user_id_a = ? AND user_id_b = ?");
    stmt.bind(1, a);
    stmt.bind(2, b);
    stmt.exec();
}

bool FriendshipDao::isFriend(int64_t userA, int64_t userB) {
    auto [a, b] = ordered(userA, userB);
    SQLite::Statement stmt(db_,
        "SELECT 1 FROM friendships WHERE user_id_a = ? AND user_id_b = ?");
    stmt.bind(1, a);
    stmt.bind(2, b);
    return stmt.executeStep();
}

std::vector<int64_t> FriendshipDao::findFriends(int64_t userId) {
    std::vector<int64_t> friends;

    // userId 作为 a
    SQLite::Statement s1(db_,
        "SELECT user_id_b FROM friendships WHERE user_id_a = ?");
    s1.bind(1, userId);
    while (s1.executeStep()) {
        friends.push_back(s1.getColumn(0).getInt64());
    }

    // userId 作为 b
    SQLite::Statement s2(db_,
        "SELECT user_id_a FROM friendships WHERE user_id_b = ?");
    s2.bind(1, userId);
    while (s2.executeStep()) {
        friends.push_back(s2.getColumn(0).getInt64());
    }

    return friends;
}

} // namespace storage
} // namespace wechat
