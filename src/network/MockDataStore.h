#pragma once

#include <wechat/core/Group.h>
#include <wechat/core/Message.h>
#include <wechat/core/User.h>
#include <wechat/network/MomentService.h>
#include <wechat/storage/DatabaseManager.h>
#include <wechat/storage/FriendshipDao.h>
#include <wechat/storage/GroupDao.h>
#include <wechat/storage/MessageDao.h>
#include <wechat/storage/UserDao.h>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
namespace wechat { namespace network {
/// Mock 服务端存储
/// 内部使用 storage 模块 (SQLite :memory:) 持久化用户/好友/群组/消息
/// 认证 token 和朋友圈仍为内存管理（无对应 DAO）
/// 所有 MockXxxService 共享同一个 MockDataStore 实例
class MockDataStore {
public:
    MockDataStore();

    // ── 时间 ──
     int64_t now();

    // ── 用户 / 认证 ──

    /// 注册，返回 userId
    int64_t addUser(const std::string& username, const std::string& password);
    /// 验证密码，返回 userId（0 = 失败）
    int64_t authenticate(const std::string& username, const std::string& password);
    /// 创建 token
    std::string createToken(int64_t userId);
    /// token -> userId（0 = 无效）
    int64_t resolveToken(const std::string& token);
    /// 删除 token
    void removeToken(const std::string& token);
    /// 查找用户
    core::User* findUser(int64_t userId);
    /// 按关键字搜索用户
    std::vector<core::User> searchUsers(const std::string& keyword);

    // ── 好友 ──

    void addFriendship(int64_t a, int64_t b);
    void removeFriendship(int64_t a, int64_t b);
    bool areFriends(int64_t a, int64_t b);
    std::vector<int64_t> getFriendIds(int64_t userId);

    // ── 群组 ──

    core::Group& createGroup(int64_t ownerId,
                             const std::vector<int64_t>& memberIds);
    core::Group* findGroup(int64_t groupId);
    void addGroupMember(int64_t groupId, int64_t userId);
    void removeGroupMember(int64_t groupId, int64_t userId);
    void removeGroup(int64_t groupId);
    std::vector<core::Group> getGroupsByUser(int64_t userId);

    // ── 消息 ──

    core::Message& addMessage(int64_t senderId,
                              int64_t chatId,
                              int64_t replyTo,
                              const core::MessageContent& content);
    core::Message* findMessage(int64_t messageId);
    /// 将修改后的消息写回存储（findMessage 返回的指针修改后需调用）
    void saveMessage(const core::Message& msg);
    /// afterId=0 → 返回最新的 limit 条（从末尾倒数），升序返回
    /// afterId>0 → 返回 id > afterId 的前 limit 条，升序返回
    std::vector<core::Message> getMessagesAfter(int64_t chatId,
                                                int64_t afterId, int limit);
    /// beforeId=0 → 返回最早的 limit 条（从头开始），升序返回
    /// beforeId>0 → 返回 id < beforeId 的最后 limit 条，升序返回
    std::vector<core::Message> getMessagesBefore(int64_t chatId,
                                                 int64_t beforeId, int limit);
    /// 获取 chatId 中 id ∈ [startId, endId] 且 updated_at > updatedAt 的消息
    std::vector<core::Message> getMessagesUpdatedAfter(int64_t chatId,
                                                       int64_t startId, int64_t endId,
                                                       int64_t updatedAt, int limit);

    // ── 朋友圈 ──

    Moment& addMoment(int64_t authorId,
                      const std::string& text,
                      const std::vector<std::string>& imageIds);
    Moment* findMoment(int64_t momentId);
    std::vector<Moment> getMoments(const std::set<int64_t>& visibleUserIds,
                                   int64_t beforeTs, int limit);

private:
    // ── SQLite 存储 ──
    storage::DatabaseManager dbm_;
    storage::UserDao userDao_;
    storage::FriendshipDao friendshipDao_;
    storage::GroupDao groupDao_;
    storage::MessageDao messageDao_;

    // ── 内存辅助（认证 + 返回指针/引用的缓存）──
    // username -> password
    std::map<std::string, std::string> passwords_;
    // userId -> username
    std::map<int64_t, std::string> userIdToName_;
    // token -> userId
    std::map<std::string, int64_t> tokens_;
    // findUser 返回指针需要稳定地址
    mutable std::map<int64_t, core::User> userCache_;
    // findGroup / createGroup 返回指针/引用需要稳定地址
    std::map<int64_t, core::Group> groupCache_;
    // findMessage / addMessage 返回指针/引用需要稳定地址
    std::map<int64_t, core::Message> messageCache_;

    // ── 朋友圈（无 DAO，纯内存）──
    std::map<int64_t, Moment> moments_;
    std::vector<int64_t> momentTimeline_;
};

} } // namespace wechat::network
