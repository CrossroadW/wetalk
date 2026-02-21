# 数据缓存机制

> 客户端通过 `[start, end]` 区间管理本地缓存，按需双向加载消息。数据模型详见 [data-models.md](./data-models.md)

## 架构概览

```
┌──────────────────────────────────────────────────────────┐
│  UI 层 (ChatWidget / MessageListView)                    │
│       ↑ 本地同步：Presenter Qt signals                    │
├──────────────────────────────────────────────────────────┤
│  Presenter 层 (ChatPresenter)                            │
│       │ 管理每个聊天的 SyncCursor [start, end]            │
│       ↑ 远程同步：fetch / fetchUpdated / 实时推送          │
├──────────────────────────────────────────────────────────┤
│  服务层 (ChatService)                                    │
│       │ 统一接口，Mock 实现 / 未来 gRPC                    │
├──────────────────────────────────────────────────────────┤
│  远程数据库 (MockDataStore / 未来真实后端)                  │
│       完整消息存储                                        │
└──────────────────────────────────────────────────────────┘
```

两个数据库：
- **远程数据库**：完整消息存储（当前由 MockDataStore 模拟）
- **客户端数据库**：本地缓存（当前由 Presenter 内存 cursor 管理，未来 SQLite）

两个同步，使用同一套语义接口：
- **远程 → 本地**：fetch / fetchUpdated / 实时推送
- **本地 → UI**：Presenter Qt signals → ChatWidget

## 缓存区间 `[start, end]`

每个聊天窗口维护一个 `SyncCursor { start, end }`，表示本地已缓存的消息 ID 区间。

```
远程消息序列：[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, ..., 95, 96, 97, 98, 99, 100]
                                                         ↑               ↑
客户端缓存区间：                                        start=95       end=100
                                                     [95, 96, 97, 98, 99, 100]
```

- `start` 和 `end` 都是消息 ID（全局自增，按 chatId 过滤）
- 区间内的消息是连续的（相对于该聊天）
- `start=0, end=0` 表示尚未缓存任何消息

## 三种同步方式

### 1. fetch — 按需分页加载

两个方向，统一接口：

#### `fetch(after_id, limit)`

- `after_id = 0`：特殊含义，返回**最新的** limit 条消息（从末尾倒数）
- `after_id > 0`：返回 `id > after_id` 的前 limit 条消息（升序）

触发时机：
- **打开聊天时**（`after_id=0`）：加载最新消息，初始化 `[start, end]`

#### `fetch(before_id, limit)`

- `before_id = 0`：特殊含义，返回**最早的** limit 条消息（从头开始）
- `before_id > 0`：返回 `id < before_id` 的最后 limit 条消息（降序取，升序返回）

触发时机：
- **向上滚动到边界时**（`before_id=start`）：加载更早的历史消息，更新 `start`

#### 分页流程图

```
打开聊天（本地无缓存）:
    fetch(after_id=0, limit=20)
        → 远程返回: [81, 82, ..., 100]
        → 缓存区间: start=81, end=100

向上滚动到顶部:
    fetch(before_id=81, limit=20)
        → 远程返回: [61, 62, ..., 80]
        → 缓存区间: start=61, end=100

继续向上滚动:
    fetch(before_id=61, limit=20)
        → 远程返回: [41, 42, ..., 60]
        → 缓存区间: start=41, end=100
```

### 2. fetchUpdated — 增量更新已缓存消息

```
fetchUpdated(chatId, updatedAt, limit)
```

- `updatedAt`：客户端缓存的消息表中最大的 `updated_at` 时间戳
- `updatedAt = 0`：表示本地没有任何消息被更新过
- 返回所有 `updated_at > updatedAt` 且 `id ∈ [start, end]`（本地已缓存）的消息最新版本

触发时机：
- **每次 fetch 完成后自动触发**：检查已缓存消息是否有更新（撤回、编辑、已读数等）

只关心本地已缓存的消息，未缓存的消息不管。

```
fetch 完成后:
    本地缓存 [81..100]，最大 updatedAt = 1700000000
        ↓
    fetchUpdated(chatId, updatedAt=1700000000)
        ↓
    远程返回: [msg#85(已撤回), msg#92(已编辑)]
        ↓
    更新本地缓存 + UI
```

### 3. 实时推送 — 在线期间被动接收

在线期间，通过推送通知（当前由 Boost.Signals2 模拟，未来 WebSocket）被动接收：

- **新消息**：`onMessageStored(chatId)` → 自动 `fetchAfter(end)` → 更新 `end` + UI
- **消息变更**：`onMessageUpdated(chatId, messageId)` → 拉取最新版本 → 更新 UI

```
在线期间收到新消息:
    onMessageStored("chat_1")
        → fetchAfter(end=100, limit=50)
        → 远程返回: [101]
        → end=101, UI 追加

在线期间消息被撤回:
    onMessageUpdated("chat_1", 85)
        → 拉取 msg#85 最新版本
        → UI 更新（显示"消息已撤回"）
```

## 完整数据流

```
┌─ 打开聊天 ──────────────────────────────────────────────┐
│  initChat()                                             │
│    → openChat(chatId)          // 创建 cursor           │
│    → loadLatest(chatId, 20)    // fetch(after_id=0)     │
│    → fetchUpdated(updatedAt)   // 检查已缓存消息更新      │
│    → messagesInserted → UI     // scrollToBottom         │
└─────────────────────────────────────────────────────────┘

┌─ 向上滚动加载历史 ──────────────────────────────────────┐
│  onReachedTop()                                         │
│    → loadHistory(chatId, 20)   // fetch(before_id=start)│
│    → fetchUpdated(updatedAt)   // 检查已缓存消息更新      │
│    → messagesInserted ��� UI     // 保持滚动位置            │
└─────────────────────────────────────────────────────────┘

┌─ 在线实时接收 ──────────────────────────────────────────┐
│  onMessageStored(chatId)                                │
│    → fetchAfter(end)           // 拉取新消息             │
│    → messagesInserted → UI     // 在底部则 scrollToBottom │
│                                                         │
│  onMessageUpdated(chatId, msgId)                        │
│    → 拉取 msg 最新版本                                   │
│    → messageUpdated → UI       // upsert 更新            │
└─────────────────────────────────────────────────────────┘

┌─ 发送消息 ──────────────────────────────────────────────┐
│  sendMessage()                                          │
│    → ChatService.sendMessage                            │
│    → onMessageStored 触发      // 走实时接收流程          │
└─────────────────────────────────────────────────────────┘
```

## 设计要点

1. **懒加载**：只在打开聊天和滚动到边界时 fetch，不预加载
2. **在线期间不主动 fetch**：新消息和变更通过实时推送被动接收
3. **fetch(after_id=0) 返回最新消息**：打开聊天时总是看到最新内容
4. **fetchUpdated 只管已缓存消息**：未缓存的消息等用户滚动到时再 fetch
5. **upsert 语义**：addMessage 创建或更新，保证幂等
6. **滚动策略与数据分离**：addMessage 不控制滚动，由 ChatWidget 根据上下文决定
