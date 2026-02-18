#include <gtest/gtest.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <wechat/core/Event.h>
#include <wechat/core/EventBus.h>
#include <wechat/core/Group.h>
#include <wechat/core/Message.h>
#include <wechat/core/User.h>

using namespace wechat::core;

TEST(EventBusTest, SubscribeAndPublish) {
    EventBus bus;
    bool called = false;

    bus.subscribe([&](Event const &e) { called = true; });

    bus.publish(MessageSentEvent{});
    EXPECT_TRUE(called);
}

TEST(EventBusTest, MultipleSubscribers) {
    EventBus bus;
    int count = 0;

    bus.subscribe([&](Event const &) { ++count; });
    bus.subscribe([&](Event const &) { ++count; });
    bus.subscribe([&](Event const &) { ++count; });

    bus.publish(MessageSentEvent{});
    EXPECT_EQ(count, 3);
}

TEST(EventBusTest, Disconnect) {
    EventBus bus;
    int count = 0;

    auto conn = bus.subscribe([&](Event const &) { ++count; });
    bus.publish(MessageSentEvent{});
    EXPECT_EQ(count, 1);

    conn.disconnect();
    bus.publish(MessageSentEvent{});
    EXPECT_EQ(count, 1);
}

TEST(EventBusTest, ScopedConnection) {
    EventBus bus;
    int count = 0;

    {
        boost::signals2::scoped_connection sc =
            bus.subscribe([&](Event const &) { ++count; });
        bus.publish(MessageSentEvent{});
        EXPECT_EQ(count, 1);
    }

    bus.publish(MessageSentEvent{});
    EXPECT_EQ(count, 1);
}

TEST(EventBusTest, SubscriberCount) {
    EventBus bus;
    EXPECT_EQ(bus.subscriberCount(), 0);

    auto c1 = bus.subscribe([](Event const &) {});
    EXPECT_EQ(bus.subscriberCount(), 1);

    auto c2 = bus.subscribe([](Event const &) {});
    EXPECT_EQ(bus.subscriberCount(), 2);

    c1.disconnect();
    EXPECT_EQ(bus.subscriberCount(), 1);
}

TEST(EventBusTest, NoSubscribersPublishDoesNotCrash) {
    EventBus bus;
    EXPECT_NO_THROW(bus.publish(MessageSentEvent{}));
    EXPECT_EQ(bus.subscriberCount(), 0);
}

// ── SQLite 基础测试 ──

class SQLiteTest : public ::testing::Test {
protected:
    SQLite::Database db{"", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE |
                                SQLite::OPEN_MEMORY};

    void SetUp() override {
        db.exec("CREATE TABLE users ("
                "    id TEXT PRIMARY KEY"
                ");"
                "CREATE TABLE groups_ ("
                "    id TEXT PRIMARY KEY,"
                "    owner_id TEXT"
                ");"
                "CREATE TABLE group_members ("
                "    group_id TEXT,"
                "    user_id TEXT,"
                "    PRIMARY KEY (group_id, user_id)"
                ");"
                "CREATE TABLE messages ("
                "    id TEXT PRIMARY KEY,"
                "    sender_id TEXT,"
                "    chat_id TEXT NOT NULL,"
                "    reply_to TEXT,"
                "    content_data TEXT NOT NULL,"
                "    timestamp INTEGER NOT NULL,"
                "    edited_at INTEGER DEFAULT 0,"
                "    revoked INTEGER DEFAULT 0,"
                "    read_count INTEGER DEFAULT 0"
                ");");
    }
};

TEST_F(SQLiteTest, InsertAndQueryUser) {
    SQLite::Statement insert(db, "INSERT INTO users (id) VALUES (?)");
    insert.bind(1, "user1");
    insert.exec();

    SQLite::Statement query(db, "SELECT id FROM users WHERE id = ?");
    query.bind(1, "user1");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(query.getColumn(0).getString(), "user1");
}

TEST_F(SQLiteTest, InsertAndQueryGroup) {
    db.exec("INSERT INTO users (id) VALUES ('u1')");
    db.exec("INSERT INTO users (id) VALUES ('u2')");

    SQLite::Statement insertGroup(
        db, "INSERT INTO groups_ (id, owner_id) VALUES (?, ?)");
    insertGroup.bind(1, "g1");
    insertGroup.bind(2, "");
    insertGroup.exec();

    SQLite::Statement insertMember(
        db, "INSERT INTO group_members (group_id, user_id) VALUES (?, ?)");
    insertMember.bind(1, "g1");
    insertMember.bind(2, "u1");
    insertMember.exec();
    insertMember.reset();
    insertMember.bind(1, "g1");
    insertMember.bind(2, "u2");
    insertMember.exec();

    SQLite::Statement countMembers(
        db, "SELECT COUNT(*) FROM group_members WHERE group_id = ?");
    countMembers.bind(1, "g1");
    ASSERT_TRUE(countMembers.executeStep());
    EXPECT_EQ(countMembers.getColumn(0).getInt(), 2);
}

TEST_F(SQLiteTest, InsertAndQueryMessage) {
    db.exec("INSERT INTO groups_ (id, owner_id) VALUES ('g1', '')");

    SQLite::Statement insert(db, "INSERT INTO messages (id, sender_id, "
                                 "chat_id, reply_to, content_data, timestamp) "
                                 "VALUES (?, ?, ?, ?, ?, ?)");
    insert.bind(1, "msg1");
    insert.bind(2, "u1");
    insert.bind(3, "g1");
    insert.bind(4); // NULL reply_to
    insert.bind(5, R"([{"type":1,"data":{"text":"hello"}}])");
    insert.bind(6, 1'700'000'000);
    insert.exec();

    SQLite::Statement query(
        db, "SELECT * FROM messages WHERE chat_id = ? ORDER BY timestamp");
    query.bind(1, "g1");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(query.getColumn("id").getString(), "msg1");
    EXPECT_EQ(query.getColumn("sender_id").getString(), "u1");
    EXPECT_EQ(query.getColumn("content_data").getString(),
              R"([{"type":1,"data":{"text":"hello"}}])");
    EXPECT_EQ(query.getColumn("timestamp").getInt64(), 1'700'000'000);
    EXPECT_TRUE(query.getColumn("reply_to").isNull());
    EXPECT_EQ(query.getColumn("revoked").getInt(), 0);
    EXPECT_EQ(query.getColumn("read_count").getInt(), 0);
}

TEST_F(SQLiteTest, MessageEditAndRevoke) {
    db.exec("INSERT INTO groups_ (id, owner_id) VALUES ('g1', '')");
    db.exec("INSERT INTO messages (id, sender_id, chat_id, content_data, "
            "timestamp) "
            "VALUES ('msg1', 'u1', 'g1', "
            "'[{\"type\":1,\"data\":{\"text\":\"hello\"}}]', 1700000000)");

    // 编辑
    SQLite::Statement edit(
        db, "UPDATE messages SET content_data = ?, edited_at = ? WHERE id = ?");
    edit.bind(1, R"([{"type":1,"data":{"text":"hello edited"}}])");
    edit.bind(2, 1'700'000'100);
    edit.bind(3, "msg1");
    edit.exec();

    SQLite::Statement q1(
        db, "SELECT content_data, edited_at FROM messages WHERE id = ?");
    q1.bind(1, "msg1");
    ASSERT_TRUE(q1.executeStep());
    EXPECT_EQ(q1.getColumn("content_data").getString(),
              R"([{"type":1,"data":{"text":"hello edited"}}])");
    EXPECT_EQ(q1.getColumn("edited_at").getInt64(), 1'700'000'100);

    // 撤回
    db.exec("UPDATE messages SET revoked = 1 WHERE id = 'msg1'");

    SQLite::Statement q2(db, "SELECT revoked FROM messages WHERE id = ?");
    q2.bind(1, "msg1");
    ASSERT_TRUE(q2.executeStep());
    EXPECT_EQ(q2.getColumn("revoked").getInt(), 1);
}

TEST_F(SQLiteTest, MessageReplyTo) {
    db.exec("INSERT INTO groups_ (id, owner_id) VALUES ('g1', '')");
    db.exec("INSERT INTO messages (id, sender_id, chat_id, content_data, "
            "timestamp) "
            "VALUES ('msg1', 'u1', 'g1', "
            "'[{\"type\":1,\"data\":{\"text\":\"original\"}}]', 1700000000)");

    // 引用消息
    SQLite::Statement insert(db, "INSERT INTO messages (id, sender_id, "
                                 "chat_id, reply_to, content_data, timestamp) "
                                 "VALUES (?, ?, ?, ?, ?, ?)");
    insert.bind(1, "msg2");
    insert.bind(2, "u2");
    insert.bind(3, "g1");
    insert.bind(4, "msg1");
    insert.bind(5, R"([{"type":1,"data":{"text":"reply"}}])");
    insert.bind(6, 1'700'000'001);
    insert.exec();

    SQLite::Statement query(db, "SELECT reply_to FROM messages WHERE id = ?");
    query.bind(1, "msg2");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(query.getColumn("reply_to").getString(), "msg1");
}

TEST_F(SQLiteTest, ReadCountIncrement) {
    db.exec("INSERT INTO groups_ (id, owner_id) VALUES ('g1', '')");
    db.exec("INSERT INTO messages (id, sender_id, chat_id, content_data, "
            "timestamp) "
            "VALUES ('msg1', 'u1', 'g1', "
            "'[{\"type\":1,\"data\":{\"text\":\"hi\"}}]', 1700000000)");

    db.exec(
        "UPDATE messages SET read_count = read_count + 1 WHERE id = 'msg1'");
    db.exec(
        "UPDATE messages SET read_count = read_count + 1 WHERE id = 'msg1'");
    db.exec(
        "UPDATE messages SET read_count = read_count + 1 WHERE id = 'msg1'");

    SQLite::Statement query(db, "SELECT read_count FROM messages WHERE id = ?");
    query.bind(1, "msg1");
    ASSERT_TRUE(query.executeStep());
    EXPECT_EQ(query.getColumn(0).getInt(), 3);
}

TEST_F(SQLiteTest, Transaction) {
    SQLite::Transaction tx(db);
    db.exec("INSERT INTO users (id) VALUES ('u1')");
    db.exec("INSERT INTO users (id) VALUES ('u2')");
    tx.commit();

    SQLite::Statement count(db, "SELECT COUNT(*) FROM users");
    ASSERT_TRUE(count.executeStep());
    EXPECT_EQ(count.getColumn(0).getInt(), 2);
}

TEST_F(SQLiteTest, TransactionRollback) {
    {
        SQLite::Transaction tx(db);
        db.exec("INSERT INTO users (id) VALUES ('u1')");
        // tx 析构不 commit → 自动回滚
    }

    SQLite::Statement count(db, "SELECT COUNT(*) FROM users");
    ASSERT_TRUE(count.executeStep());
    EXPECT_EQ(count.getColumn(0).getInt(), 0);
}
