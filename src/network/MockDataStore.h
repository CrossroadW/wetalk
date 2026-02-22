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

    // ── ID 生成 ──
    /// 生成递增 ID（模拟 SQLite rowid）
    int64_t nextId();

    // ── 用户 / 认证 ──

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
    void addGroupMember(const std::string& groupId, const std::string& userId);
    void removeGroupMember(const std::string& groupId, const std::string& userId);
    void removeGroup(const std::string& groupId);
    std::vector<core::Group> getGroupsByUser(const std::string& userId);

    // ── 消息 ──

    core::Message& addMessage(const std::string& senderId,
                              const std::string& chatId,
                              int64_t replyTo,
                              const core::MessageContent& content);
    core::Message* findMessage(int64_t messageId);
    /// 将修改后的消息写回存储（findMessage 返回的指针修改后需调用）
    void saveMessage(const core::Message& msg);
    /// afterId=0 → 返回最新的 limit 条（从末尾倒数），升序返回
    /// afterId>0 → 返回 id > afterId 的前 limit 条，升序返回
    std::vector<core::Message> getMessagesAfter(const std::string& chatId,
                                                int64_t afterId, int limit);
    /// beforeId=0 → 返回最早的 limit 条（从头开始），升序返回
    /// beforeId>0 → 返回 id < beforeId 的最后 limit 条，升序返回
    std::vector<core::Message> getMessagesBefore(const std::string& chatId,
                                                 int64_t beforeId, int limit);
    /// 获取 chatId 中 id ∈ [startId, endId] 且 updated_at > updatedAt 的消息
    std::vector<core::Message> getMessagesUpdatedAfter(const std::string& chatId,
                                                       int64_t startId, int64_t endId,
                                                       int64_t updatedAt, int limit);

    // ── 朋友圈 ──

    Moment& addMoment(const std::string& authorId,
                      const std::string& text,
                      const std::vector<std::string>& imageIds);
    Moment* findMoment(int64_t momentId);
    std::vector<Moment> getMoments(const std::set<std::string>& visibleUserIds,
                                   int64_t beforeTs, int limit);

private:
    int64_t idCounter;

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
    std::map<std::string, std::string> userIdToName_;
    // token -> userId
    std::map<std::string, std::string> tokens_;
    // findUser 返回指针需要稳定地址
    mutable std::map<std::string, core::User> userCache_;
    // findGroup / createGroup 返回指针/引用需要稳定地址
    std::map<std::string, core::Group> groupCache_;
    // findMessage / addMessage 返回指针/引用需要稳定地址
    std::map<int64_t, core::Message> messageCache_;

    // ── 朋友圈（无 DAO，纯内存）──
    std::map<int64_t, Moment> moments_;
    std::vector<int64_t> momentTimeline_;
};

} } // namespace wechat::network
