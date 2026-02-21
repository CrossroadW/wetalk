# 缓存同步重构方案

> 基于 [data-cache-mechanism.md](./data-cache-mechanism.md) 的设计，对现有代码进行重构。
> 核心变更：新增 `fetchUpdated` 接口、修改 `fetchAfter(0)` 语义为返回最新消息、打开聊天改用 `loadLatest`。

## 与现有实现的差异

| 项目 | 现状 | 目标 |
|------|------|------|
| `fetchAfter(afterId=0)` | 返回最早的消息 | 返回**最新的**消息（从末尾倒数） |
| 打开聊天初始加载 | `loadHistory` → `fetchBefore(INT64_MAX)` | `loadLatest` → `fetchAfter(0)` |
| `fetchUpdated` | 不存在 | 新增，每次 fetch 后自动触发 |
| `onMessageStored` 推送 | `fetchAfter(cursor.end)` | 不变 |
| `onMessageUpdated` 推送 | 单条 fetch | 不变 |
| `before_id=0` | 未定义 | 返回最早的消息（从头开始） |

## 改动清单

### 1. ChatService 接口 — 新增 `fetchUpdated`

文件：`include/wechat/network/ChatService.h`

```cpp
/// 返回 chatId 中 updated_at > updatedAt 的消息（只返回 id ∈ 已缓存范围的）
/// updatedAt = 0 表示本地无更新记录，返回所有有 updated_at > 0 的已缓存消息
virtual Result<SyncMessagesResponse> fetchUpdated(
    const std::string& token, const std::string& chatId,
    int64_t startId, int64_t endId,
    int64_t updatedAt, int limit) = 0;
```

参数说明：
- `startId, endId`：客户端当前缓存区间，服务端只返回此范围内的消息
- `updatedAt`：客户端已知的最大 `updated_at`，服务端返回比这个更新的
- `limit`：分页限制

### 2. MockDataStore — 新增 `getMessagesUpdatedAfter` + 修改 `getMessagesAfter`

文件：`src/network/MockDataStore.h` / `.cpp`

```cpp
// 新增：获取 chatId 中 id ∈ [startId, endId] 且 updated_at > updatedAt 的消息
std::vector<core::Message> getMessagesUpdatedAfter(
    const std::string& chatId, int64_t startId, int64_t endId,
    int64_t updatedAt, int limit);
```

修改 `getMessagesAfter` 语义：
```cpp
// afterId = 0 时，返回最新的 limit 条（从末尾倒数），而非从头开始
std::vector<core::Message> getMessagesAfter(
    const std::string& chatId, int64_t afterId, int limit);
```

实现要点：
- `afterId = 0`：取 `chatMessages[chatId]` 的最后 limit 条，升序返回
- `afterId > 0`：保持现有逻辑，返回 `id > afterId` 的前 limit 条

同理修改 `getMessagesBefore`：
- `beforeId = 0`：取 `chatMessages[chatId]` 的前 limit 条，升序返回
- `beforeId > 0`：保持现有逻辑

### 3. MockChatService — 实现 `fetchUpdated` + 适配新语义

文件：`src/network/MockChatService.h` / `.cpp`

```cpp
Result<SyncMessagesResponse> MockChatService::fetchUpdated(
    const std::string& token, const std::string& chatId,
    int64_t startId, int64_t endId,
    int64_t updatedAt, int limit) {
    auto userId = store->resolveToken(token);
    if (userId.empty())
        return {ErrorCode::Unauthorized, "invalid token"};

    auto msgs = store->getMessagesUpdatedAfter(chatId, startId, endId, updatedAt, limit + 1);
    bool hasMore = static_cast<int>(msgs.size()) > limit;
    if (hasMore) msgs.pop_back();

    return SyncMessagesResponse{std::move(msgs), hasMore};
}
```

### 4. ChatPresenter — 新增 `loadLatest` + `syncUpdated`

文件：`include/wechat/chat/ChatPresenter.h` / `src/chat/ChatPresenter.cpp`

新增成员：
```cpp
struct SyncCursor {
    int64_t start = 0;
    int64_t end = 0;
    int64_t maxUpdatedAt = 0;  // 新增：已缓存消息的最大 updated_at
};
```

新增方法：
```cpp
/// 打开聊天时调用，fetch(after_id=0) 加载最新消息
void loadLatest(std::string const& chatId, int limit = 20);

/// 每次 fetch 后自动调用，检查已缓存消息的更新
void syncUpdated(std::string const& chatId);
```

实现：
```cpp
void ChatPresenter::loadLatest(std::string const& chatId, int limit) {
    if (chatId.empty() || token_.empty()) return;

    auto& cursor = cursors_[chatId];
    // after_id=0 → 返回最新 limit 条
    auto result = client_.chat().fetchAfter(token_, chatId, 0, limit);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.start = msgs.front().id;
        cursor.end = msgs.back().id;
        updateMaxUpdatedAt(cursor, msgs);
        Q_EMIT messagesInserted(QString::fromStdString(chatId), msgs);
        syncUpdated(chatId);  // fetch 后自动检查更新
    }
}

void ChatPresenter::loadHistory(std::string const& chatId, int limit) {
    if (chatId.empty() || token_.empty()) return;

    auto& cursor = cursors_[chatId];
    int64_t beforeId = cursor.start > 0 ? cursor.start : 0;
    // before_id=0 → 返回最早的消息（不太可能走到，因为 loadLatest 先执行）
    auto result = client_.chat().fetchBefore(token_, chatId, beforeId, limit);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        cursor.start = msgs.front().id;
        if (cursor.end == 0) cursor.end = msgs.back().id;
        updateMaxUpdatedAt(cursor, msgs);
        Q_EMIT messagesInserted(QString::fromStdString(chatId), msgs);
        syncUpdated(chatId);  // fetch 后自动检查更新
    }
}

void ChatPresenter::syncUpdated(std::string const& chatId) {
    if (chatId.empty() || token_.empty()) return;

    auto& cursor = cursors_[chatId];
    if (cursor.start == 0 || cursor.end == 0) return;  // 无缓存

    auto result = client_.chat().fetchUpdated(
        token_, chatId, cursor.start, cursor.end, cursor.maxUpdatedAt, 100);

    if (result.ok() && !result.value().messages.empty()) {
        auto& msgs = result.value().messages;
        updateMaxUpdatedAt(cursor, msgs);
        for (auto const& msg : msgs) {
            Q_EMIT messageUpdated(QString::fromStdString(chatId), msg);
        }
    }
}

// 辅助：更新 cursor 的 maxUpdatedAt
void ChatPresenter::updateMaxUpdatedAt(SyncCursor& cursor,
                                        std::vector<core::Message> const& msgs) {
    for (auto const& msg : msgs) {
        if (msg.updatedAt > cursor.maxUpdatedAt) {
            cursor.maxUpdatedAt = msg.updatedAt;
        }
    }
}
```

### 5. ChatWidget — `initChat` 改用 `loadLatest`

文件：`src/chat/ChatWidget.cpp`

```cpp
void ChatWidget::initChat() {
    if (!presenter_ || chatId_.empty() || initialized_) return;
    initialized_ = true;
    presenter_->openChat(chatId_);
    presenter_->loadLatest(chatId_, 20);  // 改为 loadLatest
}
```

`onReachedTop` 保持不变（仍用 `loadHistory`）。

### 6. 测试适配

文件：`src/chat/tests/test_chat.cpp`

- 新增 `FetchUpdatedReturnsModifiedMessages` 测试
- 新增 `LoadLatestReturnsMostRecentMessages` 测试
- 修改 `LoadHistoryAfterOpenChatEmitsMessages` → 使用 `loadLatest`
- 新增 `FetchAfterZeroReturnsLatest` 测试（验证 MockDataStore 新语义）
- 新增 `FetchBeforeZeroReturnsEarliest` 测试

## 数据流对比

### 现有流程
```
initChat → openChat → loadHistory(fetchBefore INT64_MAX) → messagesInserted
```

### 重构后流程
```
initChat → openChat → loadLatest(fetchAfter 0) → messagesInserted → syncUpdated → messageUpdated(如有)
```

## 不变的部分

- `openChat` 语义不变（只创建 cursor）
- `onNetworkMessageStored` 不变（fetchAfter(end) 拉取新消息）
- `onNetworkMessageUpdated` 不变（单条拉取更新）
- `MessageListView::addMessage` upsert 语义不变
- 滚动策略不变（ChatWidget 控制）
- `onReachedTop` → `loadHistory(fetchBefore start)` 不变

### 7. 模拟后端 — MockBackend 替代 MockAutoResponder

现有 `MockAutoResponder` 只能随机发消息，无法覆盖撤回、编辑、离线堆积等场景。
用 `MockBackend` 替代，提供脚本化的模拟行为，方便测试完整数据流。

#### 设计

文件：`src/chat/MockBackend.h` / `MockBackend.cpp`

```cpp
namespace wechat::chat {

/// 模拟后端：以"对方用户"身份执行操作，走完整网络层路径
///
/// 支持两种模式：
/// 1. 预灌数据（同步）：在 ChatWidget 创建前批量写入历史消息
/// 2. 定时脚本（异步）：按时间线执行一系列动作（发消息、撤回、编辑）
///
/// 所有操作通过 ChatService 接口完成，ChatPresenter 正常感知推送通知。
class MockBackend : public QObject {
    Q_OBJECT

public:
    explicit MockBackend(network::NetworkClient& client,
                         QObject* parent = nullptr);

    /// 设置模拟用户会话
    void setPeerSession(std::string const& token,
                        std::string const& userId);

    /// 设置目标聊天
    void setChatId(std::string const& chatId);

    // ── 预灌数据（同步，ChatWidget 创建前调用）──

    /// 批量写入历史消息，返回写入的消息 ID 列表
    /// senderTokens 交替使用，模拟双方对话
    std::vector<int64_t> prefill(int count,
                                 std::vector<std::string> const& senderTokens);

    // ── 定时脚本（异步，ChatWidget 创建后调用）──

    /// 动作类型
    enum class Action { Send, Revoke, Edit };

    struct ScriptEntry {
        int delayMs;          // 距上一步的延迟（ms）
        Action action;
        std::string text;     // Send/Edit 的文本内容
        int64_t targetMsgId;  // Revoke/Edit 的目标消息 ID（Send 时忽略）
    };

    /// 设置脚本并开始执行
    void runScript(std::vector<ScriptEntry> script);

    /// 停止脚本
    void stop();

    /// 便捷：生成一个典型测试脚本
    /// 发 5 条消息 → 撤回第 2 条 → 编辑第 4 条 → 再发 3 条
    static std::vector<ScriptEntry> typicalScript();

Q_SIGNALS:
    void scriptFinished();

private:
    void executeNext();

    network::NetworkClient& client_;
    std::string peerToken_;
    std::string peerId_;
    std::string chatId_;

    QTimer timer_;
    std::vector<ScriptEntry> script_;
    int scriptIndex_ = 0;
    std::vector<int64_t> sentMsgIds_;  // 脚本中发送的消息 ID，供后续撤回/编辑引用
};

} // namespace wechat::chat
```

#### 核心实现

```cpp
std::vector<int64_t> MockBackend::prefill(
    int count, std::vector<std::string> const& senderTokens) {
    std::vector<int64_t> ids;
    for (int i = 0; i < count; ++i) {
        auto const& token = senderTokens[i % senderTokens.size()];
        std::string text = "[" + std::to_string(i + 1) + "] " + kMessages[i % kMessages.size()];
        core::TextContent tc; tc.text = text;
        auto result = client_.chat().sendMessage(token, chatId_, 0, {tc});
        if (result.ok()) ids.push_back(result.value().id);
    }
    return ids;
}

void MockBackend::executeNext() {
    if (scriptIndex_ >= static_cast<int>(script_.size())) {
        Q_EMIT scriptFinished();
        return;
    }
    auto const& entry = script_[scriptIndex_++];
    switch (entry.action) {
    case Action::Send: {
        core::TextContent tc; tc.text = entry.text;
        auto result = client_.chat().sendMessage(peerToken_, chatId_, 0, {tc});
        if (result.ok()) sentMsgIds_.push_back(result.value().id);
        break;
    }
    case Action::Revoke:
        client_.chat().revokeMessage(peerToken_, entry.targetMsgId);
        break;
    case Action::Edit: {
        core::TextContent tc; tc.text = entry.text;
        client_.chat().editMessage(peerToken_, entry.targetMsgId, {tc});
        break;
    }
    }
    // 调度下一步
    if (scriptIndex_ < static_cast<int>(script_.size())) {
        timer_.start(script_[scriptIndex_].delayMs);
    } else {
        Q_EMIT scriptFinished();
    }
}

std::vector<MockBackend::ScriptEntry> MockBackend::typicalScript() {
    return {
        {500,  Action::Send,   "对方消息 1", 0},
        {300,  Action::Send,   "对方消息 2", 0},
        {800,  Action::Send,   "对方消息 3", 0},
        {400,  Action::Send,   "对方消息 4", 0},
        {600,  Action::Send,   "对方消息 5", 0},
        {1000, Action::Revoke, "",           -2},  // -2 = sentMsgIds_[1]，即"对方消息 2"
        {500,  Action::Edit,   "对方消息 4（已编辑）", -4}, // -4 = sentMsgIds_[3]
        {700,  Action::Send,   "对方消息 6", 0},
        {300,  Action::Send,   "对方消息 7", 0},
        {500,  Action::Send,   "对方消息 8", 0},
    };
}
```

`targetMsgId` 约定：
- `> 0`：直接使用消息 ID
- `< 0`：索引引用，`-n` 表示 `sentMsgIds_[n-1]`（脚本中第 n 条 Send 的消息）
- `= 0`：忽略（Send 动作）

#### ChatSandbox 适配

```cpp
// ChatSandbox::onAddChat() 中替换原有预灌逻辑：

auto backend = new MockBackend(*client_, widget);
backend->setPeerSession(peerToken, peerId);
backend->setChatId(chatId);

// 预灌历史（Presenter session 临时清除）
presenter_->setSession("", "");
backend->prefill(100, {myToken_, peerToken});
presenter_->setSession(myToken_, myUserId_);

// ChatWidget 创建后，启动测试脚本
backend->runScript(MockBackend::typicalScript());
```

#### 测试场景覆盖

| 场景 | 如何测试 |
|------|----------|
| 打开聊天加载最新消息 | `prefill(100)` → `initChat` → 验证显示最新 20 条 |
| 向上滚动加载历史 | 滚动到顶部 → 验证加载更早的 20 条 |
| 实时收到新消息 | `runScript` Send → 验证 UI 追加 |
| 消息撤回 | `runScript` Revoke → 验证 UI 更新为"已撤回" |
| 消息编辑 | `runScript` Edit → 验证 UI 内容更新 |
| fetchUpdated 增量同步 | `prefill` → 外部修改消息 → `loadLatest` → `syncUpdated` → 验证更新 |
| 离线消息堆积 | `prefill` 后不启动实时推送 → `loadLatest` 一次性拉取 |

#### 删除 MockAutoResponder

`MockAutoResponder.h` / `.cpp` 不再需要，由 `MockBackend` 完全替代。
从 CMakeLists.txt 中移除相关源文件。

## 实施顺序

1. MockDataStore：修改 `getMessagesAfter`/`getMessagesBefore` 的 0 值语义 + 新增 `getMessagesUpdatedAfter`
2. ChatService 接口：新增 `fetchUpdated`
3. MockChatService：实现 `fetchUpdated` + 适配新语义
4. ChatPresenter：新增 `loadLatest` / `syncUpdated` / `updateMaxUpdatedAt` / `SyncCursor.maxUpdatedAt`
5. ChatWidget：`initChat` 改用 `loadLatest`
6. MockBackend：新建，替代 MockAutoResponder
7. ChatSandbox：适配 MockBackend
8. 测试：适配 + 新增
