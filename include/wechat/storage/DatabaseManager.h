#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <string>

namespace wechat {
namespace storage {

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbPath);

    SQLite::Database& db();

    /// 创建所有表和索引（幂等）
    void initSchema();

private:
    std::unique_ptr<SQLite::Database> db_;
};

} // namespace storage
} // namespace wechat
