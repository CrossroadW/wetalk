#include <gtest/gtest.h>
#include "wechat/core/Message.h"
#include "wechat/storage/DatabaseManager.h"
#include "wechat/storage/UserDao.h"
#include "wechat/storage/GroupDao.h"
#include "wechat/storage/MessageDao.h"
#include "wechat/storage/FriendshipDao.h"

using namespace wechat::core;
using namespace wechat::storage;

class StorageDaoTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbm = std::make_unique<DatabaseManager>(":memory:");
        dbm->initSchema();
    }
    std::unique_ptr<DatabaseManager> dbm;
};

// ── User ──

TEST_F(StorageDaoTest, UserInsertAndFind) {
    UserDao dao(dbm->db());
    auto id1 = dao.insert(User{});
    auto id2 = dao.insert(User{});

    auto u = dao.findById(id1);
    ASSERT_TRUE(u.has_value());
    EXPECT_EQ(u->id, id1);

    EXPECT_FALSE(dao.findById(999).has_value());

    auto all = dao.findAll();
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(StorageDaoTest, UserRemove) {
    UserDao dao(dbm->db());
    auto id1 = dao.insert(User{});
    dao.remove(id1);
    EXPECT_FALSE(dao.findById(id1).has_value());
}

// ── Friendship ──

TEST_F(StorageDaoTest, FriendshipAddAndQuery) {
    FriendshipDao dao(dbm->db());
    dao.add(1, 2);
    dao.add(3, 1);

    EXPECT_TRUE(dao.isFriend(1, 2));
    EXPECT_TRUE(dao.isFriend(2, 1)); // 方向无关

    auto friends = dao.findFriends(1);
    EXPECT_EQ(friends.size(), 2u);

    dao.remove(2, 1);
    EXPECT_FALSE(dao.isFriend(1, 2));
}

// ── Group ──

TEST_F(StorageDaoTest, GroupCreateAndMembers) {
    GroupDao dao(dbm->db());

    Group g;
    g.ownerId = 1;
    g.memberIds = {1, 2, 3};
    auto gid = dao.insertGroup(g, 1000);

    auto found = dao.findGroupById(gid);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->ownerId, 1);
    EXPECT_EQ(found->memberIds.size(), 3u);

    // 用户查所属组
    auto groups = dao.findGroupIdsByUser(2);
    EXPECT_EQ(groups.size(), 1u);
    EXPECT_EQ(groups[0], gid);
}

TEST_F(StorageDaoTest, GroupAddRemoveMember) {
    GroupDao dao(dbm->db());

    auto gid = dao.insertGroup(Group{0, 1, {1}}, 1000);

    dao.addMember(gid, 4, 2000);
    auto members = dao.findMemberIds(gid);
    EXPECT_EQ(members.size(), 2u);

    // 软删除
    dao.removeMember(gid, 4, 3000);
    members = dao.findMemberIds(gid);
    EXPECT_EQ(members.size(), 1u);

    // 重新加入
    dao.addMember(gid, 4, 4000);
    members = dao.findMemberIds(gid);
    EXPECT_EQ(members.size(), 2u);
}

TEST_F(StorageDaoTest, GroupIncrementalSync) {
    GroupDao dao(dbm->db());

    dao.insertGroup(Group{0, 1, {1}}, 1000);
    auto gid2 = dao.insertGroup(Group{0, 2, {2}}, 2000);

    auto updated = dao.findGroupsUpdatedAfter(1500);
    EXPECT_EQ(updated.size(), 1u);
    EXPECT_EQ(updated[0].id, gid2);
}

// ── Message (含 JSON 序列化) ──

TEST_F(StorageDaoTest, MessageInsertAndFind) {
    MessageDao dao(dbm->db());

    Message msg;
    msg.senderId = 1;
    msg.chatId = 1;
    msg.replyTo = 0;
    msg.content = {
        TextContent{"hello"},
        ResourceContent{
            "res001",
            ResourceType::Image,
            ResourceSubtype::Jpeg,
            ResourceMeta{1024, "photo.jpg", {{"width", "800"}, {"height", "600"}}}
        }
    };
    msg.timestamp = 1000;
    msg.editedAt = 0;
    msg.revoked = false;
    msg.readCount = 0;
    msg.updatedAt = 0;

    auto id = dao.insert(msg);

    auto found = dao.findById(id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->senderId, 1);
    EXPECT_EQ(found->chatId, 1);
    EXPECT_EQ(found->content.size(), 2u);

    // 验证 TextContent
    auto* text = std::get_if<TextContent>(&found->content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "hello");

    // 验证 ResourceContent
    auto* res = std::get_if<ResourceContent>(&found->content[1]);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->resourceId, "res001");
    EXPECT_EQ(res->type, ResourceType::Image);
    EXPECT_EQ(res->subtype, ResourceSubtype::Jpeg);
    EXPECT_EQ(res->meta.size, 1024u);
    EXPECT_EQ(res->meta.filename, "photo.jpg");
    EXPECT_EQ(res->meta.extra.at("width"), "800");
}

TEST_F(StorageDaoTest, MessagePagination) {
    MessageDao dao(dbm->db());

    for (int i = 1; i <= 5; ++i) {
        Message m;
        m.senderId = 1;
        m.chatId = 1;
        m.content = {TextContent{"msg " + std::to_string(i)}};
        m.timestamp = i * 1000;
        m.editedAt = 0;
        m.revoked = false;
        m.readCount = 0;
        m.updatedAt = 0;
        dao.insert(m);
    }

    // findBefore(id=4) → id < 4 的最后 2 条，升序返回
    auto page = dao.findBefore(1, 4, 2);
    EXPECT_EQ(page.size(), 2u);
    EXPECT_EQ(page[0].id, 2); // 升序
    EXPECT_EQ(page[1].id, 3);
}

TEST_F(StorageDaoTest, MessageIncrementalSync) {
    MessageDao dao(dbm->db());

    Message m1;
    m1.senderId = 1; m1.chatId = 1;
    m1.content = {TextContent{"v1"}};
    m1.timestamp = 1000; m1.editedAt = 0;
    m1.revoked = false; m1.readCount = 0; m1.updatedAt = 0;
    auto id = dao.insert(m1);
    m1.id = id;

    // 编辑消息
    m1.content = {TextContent{"v2"}};
    m1.editedAt = 2000;
    m1.updatedAt = 2000;
    dao.update(m1);

    auto synced = dao.findUpdatedAfter(1, 1, 1, 1500, 100);
    EXPECT_EQ(synced.size(), 1u);
    auto* text = std::get_if<TextContent>(&synced[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "v2");
}

// ══════════════════════════════════════════════════
// 缓存区间 (data-cache-mechanism)
// ══════════════════════════════════════════════════

// 辅助：批量插入消息，返回插入后的 ID 列表
static std::vector<int64_t> insertMessages(MessageDao& dao, int64_t chatId,
                                           int from, int to) {
    std::vector<int64_t> ids;
    for (int i = from; i <= to; ++i) {
        Message m;
        m.senderId = 1;
        m.chatId = chatId;
        m.content = {TextContent{"msg " + std::to_string(i)}};
        m.timestamp = i * 100;  // 有序时间戳
        m.editedAt = 0;
        m.revoked = false;
        m.readCount = 0;
        m.updatedAt = 0;
        ids.push_back(dao.insert(m));
    }
    return ids;
}

TEST_F(StorageDaoTest, CacheColdStart) {
    // 首次打开聊天：findAfter(afterId=0) → 返回最新的 limit 条，升序
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 50);

    auto page = dao.findAfter(1, 0, 20);
    EXPECT_EQ(page.size(), 20u);
    EXPECT_EQ(page.front().id, ids[30]);  // 最新 20 条: ids[30]..ids[49]
    EXPECT_EQ(page.back().id, ids[49]);
}

TEST_F(StorageDaoTest, CacheScrollUp) {
    // 向上滚动：加载 start 之前的历史消息
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 50);

    // 当前缓存 start=ids[2]，向上加载 id < ids[2] 的最后 50 条，升序返回
    auto older = dao.findBefore(1, ids[2], 50);
    EXPECT_EQ(older.size(), 2u);        // ids[0], ids[1]
    EXPECT_EQ(older[0].id, ids[0]);     // 升序
    EXPECT_EQ(older[1].id, ids[1]);
}

TEST_F(StorageDaoTest, CacheScrollDown) {
    // 向下滚动：加载 end 之后的新消息
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 80);

    // 当前缓存 end=ids[49]，向下加载 id > ids[49]
    auto newer = dao.findAfter(1, ids[49], 50);
    EXPECT_EQ(newer.size(), 30u);       // ids[50]..ids[79]
    EXPECT_EQ(newer.front().id, ids[50]);
    EXPECT_EQ(newer.back().id, ids[79]);
}

TEST_F(StorageDaoTest, CacheRealtimePush) {
    // WebSocket 推送：新消息直接插入，end 更新
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 50);

    // 模拟 WS 推送新消息
    Message pushed;
    pushed.senderId = 2;
    pushed.chatId = 1;
    pushed.content = {TextContent{"realtime msg"}};
    pushed.timestamp = 5100;
    pushed.editedAt = 0;
    pushed.revoked = false;
    pushed.readCount = 0;
    pushed.updatedAt = 0;
    auto pushedId = dao.insert(pushed);

    // 验证能查到
    auto newer = dao.findAfter(1, ids[49], 10);
    EXPECT_EQ(newer.size(), 1u);
    EXPECT_EQ(newer[0].id, pushedId);

    auto* text = std::get_if<TextContent>(&newer[0].content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "realtime msg");
}

// ══════════════════════════════════════════════════
// 消息撤回 / 编辑 / 已读
// ══════════════════════════════════════════════════

TEST_F(StorageDaoTest, MessageRevoke) {
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 1);

    dao.revoke(ids[0], 5000);

    auto msg = dao.findById(ids[0]);
    ASSERT_TRUE(msg.has_value());
    EXPECT_TRUE(msg->revoked);
    EXPECT_EQ(msg->updatedAt, 5000);

    // 增量同步能感知到撤回
    auto synced = dao.findUpdatedAfter(1, 1, 1, 4000, 100);
    EXPECT_EQ(synced.size(), 1u);
    EXPECT_TRUE(synced[0].revoked);
}

TEST_F(StorageDaoTest, MessageEditContent) {
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 1);

    wechat::core::MessageContent newContent = {TextContent{"edited text"}};
    dao.editContent(ids[0], newContent, 6000);

    auto msg = dao.findById(ids[0]);
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->editedAt, 6000);
    EXPECT_EQ(msg->updatedAt, 6000);

    auto* text = std::get_if<TextContent>(&msg->content[0]);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text, "edited text");

    // 增量同步能感知到编辑
    auto synced = dao.findUpdatedAfter(1, 1, 1, 5000, 100);
    EXPECT_EQ(synced.size(), 1u);
    EXPECT_EQ(synced[0].editedAt, 6000);
}

TEST_F(StorageDaoTest, MessageReadCount) {
    MessageDao dao(dbm->db());
    auto ids = insertMessages(dao, 1, 1, 1);

    dao.updateReadCount(ids[0], 3, 7000);

    auto msg = dao.findById(ids[0]);
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->readCount, 3u);
    EXPECT_EQ(msg->updatedAt, 7000);

    // 增量同步能感知到已读变化
    auto synced = dao.findUpdatedAfter(1, 1, 1, 6000, 100);
    EXPECT_EQ(synced.size(), 1u);
    EXPECT_EQ(synced[0].readCount, 3u);
}

// ══════════════════════════════════════════════════
// 组成员变更增量同步
// ══════════════════════════════════════════════════

TEST_F(StorageDaoTest, GroupMemberChangeSync) {
    GroupDao dao(dbm->db());

    auto gid = dao.insertGroup(Group{0, 1, {1, 2}}, 1000);

    // t=2000 加入 u3
    dao.addMember(gid, 3, 2000);
    // t=3000 移除 u2
    dao.removeMember(gid, 2, 3000);

    // 从 t=1500 开始同步
    auto changes = dao.findMemberChangesAfter(1500);
    EXPECT_EQ(changes.size(), 2u);

    // 第一条：u3 加入
    EXPECT_EQ(changes[0].groupId, gid);
    EXPECT_EQ(changes[0].userId, 3);
    EXPECT_FALSE(changes[0].removed);

    // 第二条：u2 被移除
    EXPECT_EQ(changes[1].groupId, gid);
    EXPECT_EQ(changes[1].userId, 2);
    EXPECT_TRUE(changes[1].removed);
}

TEST_F(StorageDaoTest, GroupOwnerChangeSync) {
    GroupDao dao(dbm->db());

    auto gid = dao.insertGroup(Group{0, 1, {1, 2}}, 1000);

    // t=5000 转让群主
    dao.updateOwner(gid, 2, 5000);

    auto updated = dao.findGroupsUpdatedAfter(4000);
    EXPECT_EQ(updated.size(), 1u);
    EXPECT_EQ(updated[0].ownerId, 2);
}
