# 重构方案：合并 storage 模块到 network 模块

> storage 的唯一消费者就是 network（MockDataStore）。
> 直接把 SQL 写在 MockDataStore 里，删掉整个 storage 模块，减少文件数量。

## 方案：消除 DAO 层，SQL 直接写在 MockDataStore

### 步骤 1：MockDataStore 自己管理 SQLite

- `MockDataStore.h` 移除所有 `#include <wechat/storage/...>`
- 直接持有 `SQLite::Database db_`（替代 DatabaseManager + 4 个 DAO）
- `initSchema()` 作为私有方法，构造函数中调用

### 步骤 2：内联所有 DAO SQL 到 MockDataStore.cpp

把以下文件的 SQL 逻辑全部搬进 MockDataStore.cpp：
- `UserDao.cpp` → addUser / findUser / searchUsers
- `FriendshipDao.cpp` → addFriendship / removeFriendship / areFriends / getFriendIds
- `GroupDao.cpp` → createGroup / findGroup / addGroupMember / removeGroupMember / removeGroup / getGroupsByUser
- `MessageDao.cpp` → addMessage / findMessage / saveMessage / getMessagesAfter / getMessagesBefore / getMessagesUpdatedAfter
- `DatabaseManager.cpp` → initSchema（建表 SQL）

JSON 序列化函数（serializeContent / deserializeContent）也搬进 MockDataStore.cpp 作为匿名命名空间的静态函数。

### 步骤 3：更新 CMakeLists

- `src/network/CMakeLists.txt`：添加 `SQLiteCpp` 和 `nlohmann_json::nlohmann_json`，移除 `wechat_storage`
- `CMakeLists.txt`（顶层）：移除 `add_subdirectory(src/storage)`
- `src/auth/CMakeLists.txt`：移除 `wechat_storage`
- `src/chat/CMakeLists.txt`：移除 `wechat_storage`
- `src/contacts/CMakeLists.txt`：移除 `wechat_storage`
- `src/moments/CMakeLists.txt`：移除 `wechat_storage`

### 步骤 4：删除 storage 模块

- 删除 `include/wechat/storage/` 整个目录
- 删除 `src/storage/` 整个目录（含 test_storage.cpp）

## 受影响文件

| 操作 | 文件 |
|------|------|
| 重写 | `src/network/MockDataStore.h` |
| 重写 | `src/network/MockDataStore.cpp` |
| 修改 CMake | `CMakeLists.txt`（顶层） |
| 修改 CMake | `src/network/CMakeLists.txt` |
| 修改 CMake | `src/auth/CMakeLists.txt` |
| 修改 CMake | `src/chat/CMakeLists.txt` |
| 修改 CMake | `src/contacts/CMakeLists.txt` |
| 修改 CMake | `src/moments/CMakeLists.txt` |
| 删除 | `include/wechat/storage/` 整个目录 |
| 删除 | `src/storage/` 整个目录 |

## 风险评估

- **低风险**：MockDataStore 已经包含了所有 storage 调用，只是把间接调用变成直接 SQL
- **零功能变更**：纯重构，不改任何逻辑
- **命名空间不变**：MockDataStore 仍在 `wechat::network`
