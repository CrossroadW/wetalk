# ChatPresenter 重构方案：信号语义纯化 + openChat 懒加载

## 核心问题

1. `openChat` 每次调用都 emit 全部已有消息，导致 `ChatWidget` 重复 `addMessage`
2. `openChat` 用 `fetchBefore`（倒序）vs `onNetworkMessageStored` 用 `fetchAfter`（升序），顺序不一致
3. `openChat` 不应该 emit 信号 — 它不是数据模型变化

## 设计原则

- **ChatPresenter 的信号只在数据模型变化时触发**：`onNetworkMessageStored`（新消息到达）、`onNetworkMessageUpdated`（撤回/编辑）
- **openChat 不 emit 任何信号**：它只负责确保 cursor 存在，让后续的网络通知能被处理
- **ChatWidget 首次显示时主动拉取数据**：通过 `loadHistory` 懒加载，而不是被动等 Presenter 推送
- **MessageListView 判重**：`addMessage` 内部用 message.id 判重，重复则 assert 报错

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

**addMessage 判重** — 内部维护 `std::set<int64_t> messageIds_`：
```cpp
void MessageListView::addMessage(const Message& msg, const User& user) {
    Q_ASSERT_X(messageIds_.find(msg.id) == messageIds_.end(),
               "MessageListView::addMessage",
               "duplicate message id");
    messageIds_.insert(msg.id);
    // ... 原有逻辑
}
```

### 3. ChatWidget

**openChat 时机** — `ChatWidget` 在首次变为可见时（或 `setChatId` 后）调用 `presenter_->openChat(chatId_)` + `presenter_->loadHistory(chatId_, 20)` 拉取初始消息

**onMessagesInserted** — 保持不变，只处理增量消息（网络通知触发的）

新增 `initChat()` 方法：
```cpp
void ChatWidget::initChat() {
    if (!presenter_ || chatId_.empty() || initialized_) return;
    initialized_ = true;
    presenter_->openChat(chatId_);      // 确保 cursor 存在
    presenter_->loadHistory(chatId_, 20); // 懒加载最近 20 条
}
```

在 `setPresenter` 末尾或 `showEvent` 中调用 `initChat()`。

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

- `OpenChatEmitsMessagesInsertedOnExistingMessages` → 改为 `openChat` + `loadHistory`，验证 `loadHistory` emit
- `OpenChatNoSignalWhenEmpty` → `openChat` 本身不 emit，通过
- `NotificationBeforeOpenChatStillProcessed` → `onMessageStored` 仍然自动处理，通过
- `OpenMultipleChats` → 改为 `openChat` + `loadHistory` 组合验证

## 数据流总结

```
首次打开聊天:
  ChatWidget.initChat() → openChat(创建cursor) → loadHistory(fetchBefore) → messagesInserted → UI

收到新消息:
  ChatService.onMessageStored → ChatPresenter.fetchAfter → messagesInserted → ChatWidget.onMessagesInserted → UI

发送消息:
  ChatWidget → sendMessage → ChatService → onMessageStored → 同上

切换聊天:
  ChatSandbox.switchToChat → 切换 stack widget（ChatWidget 已有数据，无需重新加载）
```
