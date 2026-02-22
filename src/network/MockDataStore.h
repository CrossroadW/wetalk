#pragma once

#include <wechat/core/Group.h>
#include <wechat/core/Message.h>
#include <wechat/core/User.h>
#include <wechat/network/MomentService.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace wechat { namespace network {
/// Mock 服务端存储
/// 内部使用 SQLite :memory: 持久化用户/好友/群组/消息/朋友圈
/// 所有 MockXxxService 共享同一个 MockDataStore 实例
class MockDataStore {
public:
    MockDataStore();

    // ── 时间 ──
    int64_t now();

    // ── 用户 / 认证 ──

    int64_t addUser(const std::string& username, const std::string& password);
    int64_t authenticate(const std::string& username, const std::string& password);
    std::string createToken(int64_t userId);
    int64_t resolveToken(const std::string& token);
    void removeToken(const std::string& token);
    std::optional<core::User> findUser(int64_t userId);
    std::vector<core::User> searchUsers(const std::string& keyword);

    // ── 好友 ──

    void addFriendship(int64_t a, int64_t b);
    void removeFriendship(int64_t a, int64_t b);
    bool areFriends(int64_t a, int64_t b);
    std::vector<int64_t> getFriendIds(int64_t userId);

    // ── 群组 ──

    core::Group createGroup(int64_t ownerId,
                            const std::vector<int64_t>& memberIds);
    std::optional<core::Group> findGroup(int64_t groupId);
    void addGroupMember(int64_t groupId, int64_t userId);
    void removeGroupMember(int64_t groupId, int64_t userId);
    void removeGroup(int64_t groupId);
    std::vector<core::Group> getGroupsByUser(int64_t userId);

    // ── 消息 ──

    core::Message addMessage(int64_t senderId,
                             int64_t chatId,
                             int64_t replyTo,
                             const core::MessageContent& content);
    std::optional<core::Message> findMessage(int64_t messageId);
    void saveMessage(const core::Message& msg);
    std::vector<core::Message> getMessagesAfter(int64_t chatId,
                                                int64_t afterId, int limit);
    std::vector<core::Message> getMessagesBefore(int64_t chatId,
                                                 int64_t beforeId, int limit);
    std::vector<core::Message> getMessagesUpdatedAfter(int64_t chatId,
                                                       int64_t startId, int64_t endId,
                                                       int64_t updatedAt, int limit);

    // ── 朋友圈 ──

    Moment addMoment(int64_t authorId,
                     const std::string& text,
                     const std::vector<std::string>& imageIds);
    std::optional<Moment> findMoment(int64_t momentId);
    bool hasLiked(int64_t momentId, int64_t userId);
    void addLike(int64_t momentId, int64_t userId);
    int64_t addComment(int64_t momentId, int64_t authorId,
                       const std::string& text);
    std::vector<Moment> getMoments(const std::set<int64_t>& visibleUserIds,
                                   int64_t beforeTs, int limit);

private:
    // ── SQLite ──
    SQLite::Database db_;

    void initSchema();

    // ── 内部 SQL 辅助 ──
    static std::pair<int64_t, int64_t> orderedPair(int64_t a, int64_t b);
    std::vector<int64_t> findGroupMemberIds(int64_t groupId);
    std::vector<int64_t> findGroupIdsByUser(int64_t userId);
    core::Message rowToMessage(SQLite::Statement& stmt);
    Moment loadMoment(int64_t momentId);

};

} } // namespace wechat::network
