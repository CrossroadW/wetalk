#pragma once

#include <wechat/core/Message.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <cstdint>
#include <string>
#include <vector>

namespace wechat {
namespace cache {

/// 客户端本地消息存储（SQLite）
///
/// 只存储当前用户相关的消息子集，是 MessageCache 的持久化后端。
/// 所有操作同步执行（主线程可调，单批 20 条 < 1ms，WAL 模式）。
///
/// 与 LocalDatabase（mock 后端）完全分离：
///   LocalDatabase  → 模拟服务端，在 wechat::network 命名空间
///   MessageStore   → 客户端本地缓存，在 wechat::cache 命名空间
class MessageStore {
public:
    explicit MessageStore(const std::string& dbPath = ":memory:");

    /// upsert：id 存在则更新，不存在则插入
    void upsertMessage(const core::Message& msg);
    void upsertMessages(const std::vector<core::Message>& msgs);

    // ── 查询（按 chatSeq 分页，结果按 chatSeq 升序）──

    std::vector<core::Message> loadLatest(int64_t chatId, int limit);
    std::vector<core::Message> loadBefore(int64_t chatId, int32_t chatSeq, int limit);
    std::vector<core::Message> loadAfter(int64_t chatId, int32_t chatSeq, int limit);
    std::vector<core::Message> loadAround(int64_t chatId, int32_t chatSeq, int radius);
    std::vector<core::Message> loadUpdated(int64_t chatId, int64_t maxUpdatedAt, int limit);

    /// 当前缓存中该聊天最大的 updatedAt 值（用于 fetchUpdated 游标）
    int64_t maxUpdatedAt(int64_t chatId) const;

private:
    SQLite::Database db_;
    void initSchema();
    core::Message rowToMessage(SQLite::Statement& stmt) const;
};

} // namespace cache
} // namespace wechat
