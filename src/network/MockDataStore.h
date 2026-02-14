#pragma once

#include <wechat/core/Group.h>
#include <wechat/core/Message.h>
#include <wechat/core/User.h>
#include <wechat/network/MomentService.h>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
namespace wechat { namespace network {
/// Mock 服务端内存状态
/// 所有 MockXxxService 共享同一个 MockDataStore 实例
class MockDataStore {
public:
    MockDataStore();

    // ── 时间 ──
    int64_t now();

    // ── ID 生成 ──
    std::string nextId(const std::string& prefix);

    // ── 用户 / 认证 ──

    struct UserRecord {
        core::User user;
        std::string password;
    };

    /// 注册，返回 userId
    std::string addUser(const std::string& username, const std::string& password);
    /// 验证密码，返回 userId（空 = 失败）
    std::string authenticate(const std::string& username, const std::string& password);
    /// 创建 token
    std::string createToken(const std::string& userId);
    /// token -> userId（空 = 无效）
    std::string resolveToken(const std::string& token);
    /// 删除 token
    void removeToken(const std::string& token);
    /// 查找用户
    core::User* findUser(const std::string& userId);
    /// 按关键字搜索用户
    std::vector<core::User> searchUsers(const std::string& keyword);

    // ── 好友 ──

    void addFriendship(const std::string& a, const std::string& b);
    void removeFriendship(const std::string& a, const std::string& b);
    bool areFriends(const std::string& a, const std::string& b);
    std::vector<std::string> getFriendIds(const std::string& userId);

    // ── 群组 ──

    core::Group& createGroup(const std::string& ownerId,
                             const std::vector<std::string>& memberIds);
    core::Group* findGroup(const std::string& groupId);
    void removeGroup(const std::string& groupId);
    std::vector<core::Group> getGroupsByUser(const std::string& userId);

    // ── 消息 ──

    core::Message& addMessage(const std::string& senderId,
                              const std::string& chatId,
                              const std::string& replyTo,
                              const core::MessageContent& content);
    core::Message* findMessage(const std::string& messageId);
    std::vector<core::Message> getMessages(const std::string& chatId,
                                           int64_t sinceTs, int limit);

    // ── 朋友圈 ──

    Moment& addMoment(const std::string& authorId,
                      const std::string& text,
                      const std::vector<std::string>& imageIds);
    Moment* findMoment(const std::string& momentId);
    std::vector<Moment> getMoments(const std::set<std::string>& visibleUserIds,
                                   int64_t beforeTs, int limit);

private:
    std::mutex mutex;
    int64_t clock;
    int64_t idCounter;

    // username -> UserRecord
    std::map<std::string, UserRecord> usersByName;
    // userId -> username (反向索引)
    std::map<std::string, std::string> userIdToName;
    // token -> userId
    std::map<std::string, std::string> tokens;

    // 好友关系 (ordered pair set)
    std::set<std::pair<std::string, std::string>> friendships;

    // groupId -> Group
    std::map<std::string, core::Group> groups;

    // messageId -> Message
    std::map<std::string, core::Message> messages;
    // chatId -> [messageId...] (按时间序)
    std::map<std::string, std::vector<std::string>> chatMessages;

    // momentId -> Moment
    std::map<std::string, Moment> moments;
    // 按时间倒序的 momentId 列表
    std::vector<std::string> momentTimeline;

    static std::pair<std::string, std::string> ordered(
        const std::string& a, const std::string& b);
};

} } // namespace wechat::network
