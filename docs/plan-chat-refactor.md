# ChatPresenter 重构方案：信号语义纯化 + 懒加载

## 核心问题

1. `openChat` 每次调用都 emit 全部已有消息，导致 `ChatWidget` 重复 `addMessage`
2. `openChat` 用 `fetchBefore`（倒序）vs `onNetworkMessageStored` 用 `fetchAfter`（升序），顺序不一致
3. `openChat` 不应该 emit 信号 — 它不是数据模型变化
4. `MessageListView` 只支持添加，不支持更新已有消息（撤回/编辑）
5. 没有滚动到边界时的懒加载机制

## 设计原则

- **ChatPresenter 的信号只在数据模型变化时触发**：`onNetworkMessageStored`（新消息到达）、`onNetworkMessageUpdated`（撤回/编辑）
- **openChat 不 emit 任何信号**：它只负责确保 cursor 存在，让后续的网络通知能被处理
- **ChatWidget 首次显示时主动拉取数据**：通过 `loadHistory` 懒加载，而不是被动等 Presenter 推送
- **滚动到边界时自动 fetch**：当用户滚动到消息列表顶部/底部边界时，根据边界消息 ID 自动触发 `loadHistory`，并显示 toast 提示
- **addMessage 幂等**：如果消息已存在则更新内容（撤回/编辑），不存在则创建。不需要删除。

## 改动清单

### 1. ChatPresenter

**openChat** — 简化为只创建 cursor，不 fetch 不 emit：
```cpp
void ChatPresenter::openChat(std::string const& chatId) {
    if (cursors_.find(chatId) == cursors_.end()) {
        cursors_[chatId] = SyncCursor{};
    }
}
```

**onNetworkMessageStored** — 保持不变，只在网络通知时触发，自动创建 cursor + fetchAfter + emit

**loadHistory(chatId, limit)** — 保持不变，fetchBefore 向上翻页

### 2. MessageListView

**addMessage 改为 upsert 语义**：
- 内部维护 `std::map<int64_t, QListWidgetItem*> itemById_`
- 如果 `msg.id` 已存在 → 找到对应 `MessageItemWidget`，更新内容（文本、撤回状态、编辑标记等）
- 如果 `msg.id` 不存在 → 创建新 item 并插入到正确位置（按 id 排序）
- 方法签名不变：`void addMessage(const Message& msg, const User& user)`

```cpp
void MessageListView::addMessage(const Message& msg, const User& user) {
    auto it = itemById_.find(msg.id);
    if (it != itemById_.end()) {
        // 已存在 → 更新内容
        auto* widget = qobject_cast<MessageItemWidget*>(
            itemWidget(it->second));
        widget->updateMessage(msg);
        return;
    }
    // 不存在 → 创建并插入
    // ... 原有创建逻辑，插入后记录到 itemById_
    itemById_[msg.id] = item;
}
```

**MessageItemWidget 新增 `updateMessage()`**：更新气泡内容、撤回状态、编辑标记。

### 3. ChatWidget

**initChat()** — 首次初始化时调用：
```cpp
void ChatWidget::initChat() {
    if (!presenter_ || chatId_.empty() || initialized_) return;
    initialized_ = true;
    presenter_->openChat(chatId_);       // 确保 cursor 存在
    presenter_->loadHistory(chatId_, 20); // 懒加载最近 20 条
}
```

在 `setPresenter` 末尾调用 `initChat()`。

**滚动边界懒加载**：
- 监听 `MessageListView` 的 `scrollBar()->valueChanged`
- 当滚动到顶部（`value == minimum`）时，取当前最早消息的 id，调用 `presenter_->loadHistory(chatId_, 20)` 向上加载更多
- 显示 toast 提示（QLabel 淡入淡出或简单的定时隐藏 label）："加载历史消息..."
- 加载完成后 toast 消失

```cpp
void ChatWidget::onScrollToTop() {
    if (!presenter_ || chatId_.empty()) return;
    showToast("加载历史消息...");
    presenter_->loadHistory(chatId_, 20);
}
```

**onMessagesInserted** — 保持不变，调用 `addMessage`（upsert 语义保证不重复）

**onMessageUpdated** — 也调用 `addMessage`（upsert 会更新已有消息）：
```cpp
void ChatWidget::onMessageUpdated(QString chatId, core::Message message) {
    if (!chatId_.empty() && chatId.toStdString() != chatId_) return;
    messageListView_->addMessage(message, currentUser_);
}
```

### 4. ChatSandbox

**switchToChat** — 只切换 stack widget，不再调 `presenter_->openChat()`：
```cpp
void ChatSandbox::switchToChat(const std::string& chatId) {
    auto it = chats_.find(chatId);
    if (it == chats_.end()) return;
    chatStack_->setCurrentWidget(it->second.widget);
}
```

### 5. 测试适配

- `OpenChatEmitsMessagesInsertedOnExistingMessages` → 改为 `openChat` 不 emit，`loadHistory` emit
- `OpenChatNoSignalWhenEmpty` → `openChat` 本身不 emit，通过
- `NotificationBeforeOpenChatStillProcessed` → `onMessageStored` 仍然自动处理，`openChat` 不再重推
- `OpenMultipleChats` → 改为 `openChat` + `loadHistory` 组合验证

## Q&A：共享 ID 空间是否导致跨聊天问题？

不会。虽然所有消息共享一个全局自增 ID（`MockDataStore::idCounter`），但 `fetchAfter` / `fetchBefore` 的查询路径是：

1. 先通过 `chatMessages[chatId]` 拿到该聊天专属的消息 ID 列表
2. 再在这个列表内按 `afterId` / `beforeId` 过滤

所以 chat_1 的 cursor.end = 5 时，`fetchAfter("chat_1", 5, 50)` 只遍历 chat_1 自己的 ID 列表，不会碰到 chat_2 的消息。ID 之间的间隙（gap）是无害的，只是意味着某些 ID 属于其他聊天。

未来切换到真实数据库时，SQL 查询同样以 `chat_id` + `id` 双条件过滤：
```sql
SELECT * FROM messages WHERE chat_id = ? AND id > ? ORDER BY id ASC LIMIT ?
```

结论：当前设计无需额外处理，`chatId` 过滤已经保证了隔离性。

## 数据流总结

```
首次打开聊天:
  ChatWidget.initChat()
    → openChat(创建cursor)
    → loadHistory(fetchBefore, limit=20)
    → messagesInserted
    → addMessage(upsert) → UI

滚动到顶部加载更多:
  onScrollToTop()
    → showToast("加载历史消息...")
    → loadHistory(fetchBefore, limit=20)
    → messagesInserted
    → addMessage(upsert) → UI

收到新消息:
  ChatService.onMessageStored
    → ChatPresenter.fetchAfter
    → messagesInserted
    → addMessage(upsert) → UI

发送消息:
  ChatWidget → sendMessage → ChatService → onMessageStored → 同上

撤回/编辑:
  ChatService.onMessageUpdated
    → ChatPresenter.fetchMessage
    → messageUpdated
    → addMessage(upsert, 更新已有消息) → UI

切换聊天:
  ChatSandbox.switchToChat → 切换 stack widget（ChatWidget 已有数据，无需重新加载）
```
