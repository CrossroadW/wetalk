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
    void remove(const std::string& id);
    std::optional<core::Message> findById(const std::string& id);

    /// 按 chat_id 分页查询，按 timestamp 降序
    std::vector<core::Message> findByChat(const std::string& chatId,
                                          int64_t beforeTimestamp, int limit);

    /// 缓存区间：获取 timestamp > afterTs 的消息（升序），用于向下加载
    std::vector<core::Message> findAfter(const std::string& chatId,
                                         int64_t afterTs, int limit);

    /// 缓存区间：获取 timestamp < beforeTs 的消息（降序），用于向上加载历史
    std::vector<core::Message> findBefore(const std::string& chatId,
                                          int64_t beforeTs, int limit);

    /// 增量同步：获取某 chat 中 updated_at > since 的消息
    std::vector<core::Message> findUpdatedAfter(const std::string& chatId,
                                                int64_t since);

    /// 撤回消息
    void revoke(const std::string& id, int64_t now);

    /// 编辑消息内容
    void editContent(const std::string& id, const core::MessageContent& content,
                     int64_t now);

    /// 更新已读人数
    void updateReadCount(const std::string& id, uint32_t readCount, int64_t now);

private:
    core::Message rowToMessage(SQLite::Statement& stmt);
    SQLite::Database& db_;
};

} // namespace storage
} // namespace wechat
