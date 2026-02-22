#pragma once

#include "wechat/core/Message.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wechat {
namespace storage {

/// MessageContent <-> JSON 序列化
std::string serializeContent(const core::MessageContent& content);
core::MessageContent deserializeContent(const std::string& json);

class MessageDao {
public:
    explicit MessageDao(SQLite::Database& db);

    void insert(const core::Message& msg);
    void update(const core::Message& msg);
    void remove(int64_t id);
    std::optional<core::Message> findById(int64_t id);

    /// afterId=0 → 返回最新的 limit 条（从末尾倒数），升序返回
    /// afterId>0 → 返回 id > afterId 的前 limit 条，升序返回
    std::vector<core::Message> findAfter(const std::string& chatId,
                                         int64_t afterId, int limit);

    /// beforeId=0 → 返回最早的 limit 条（从头开始），升序返回
    /// beforeId>0 → 返回 id < beforeId 的最后 limit 条，升序返回
    std::vector<core::Message> findBefore(const std::string& chatId,
                                          int64_t beforeId, int limit);

    /// 增量同步：获取 chatId 中 id ∈ [startId, endId] 且 updated_at > since 的消息
    std::vector<core::Message> findUpdatedAfter(const std::string& chatId,
                                                int64_t startId, int64_t endId,
                                                int64_t since, int limit);

    /// 撤回消息
    void revoke(int64_t id, int64_t now);

    /// 编辑消息内容
    void editContent(int64_t id, const core::MessageContent& content,
                     int64_t now);

    /// 更新已读人数
    void updateReadCount(int64_t id, uint32_t readCount, int64_t now);

private:
    core::Message rowToMessage(SQLite::Statement& stmt);
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
