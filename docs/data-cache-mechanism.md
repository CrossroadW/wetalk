# 消息缓存机制

> 客户端通过 `RangeSet` 管理本地缓存区间，`LazyLoader` 控制双向懒加载。
> 数据模型见 [data-models.md](./data-models.md)，重构方案见 [refactor-lazy-loader.md](./refactor-lazy-loader.md)

## 核心概念

### RangeSet — 缓存区间

每个聊天维护一个 `RangeSet`，记录已加载的 `chatSeq` 区间（自动合并重叠）：

```
远程消息：  [1  2  3  4  5 ... 48 49 50 51 ... 95 96 97 98 99 100]
已加载：                              [48..52]        [95..100]
RangeSet：  [{48,52}, {95,100}]
```

- 支持多个非连续区间（跳转后产生空洞）
- `firstSeq()` / `lastSeq()` 返回当前视图边界
- `contains(chatSeq)` 检查是否已缓存

### version — 增量同步版本

- `version` 是全局单调递增整数（不用时间戳）
- 每条消息被编辑/撤回时分配新 version
- 客户端缓存记录 `maxVersion`，用于 `fetchUpdated`

## Fetch API

```
fetchLatest(chatId, limit)              → 最新 limit 条（打开聊天时）
fetchBefore(chatId, chatSeq, limit)     → chatSeq 之前的 limit 条（触顶）
fetchAfter(chatId, chatSeq, limit)      → chatSeq 之后的 limit 条（触底/新消息）
fetchAround(chatId, chatSeq, radius)    → [chatSeq-radius, chatSeq+radius]（跳转）
fetchUpdated(chatId, maxVersion, limit) → version > maxVersion 且在已缓存区间内的消息
```

## LazyLoader 三个操作

### loadOlder — 触顶加载

```
用户滚动到顶部
  → fetchBefore(chatId, firstSeq, 20)
  → RangeSet.addRange(newStart, firstSeq-1)
  → Q_EMIT messagesInserted(chatId, msgs, InsertHint::Top)
  → ChatWidget 保存锚点，前插，修正滚动条位置（防跳动）
```

### loadNewer — 触底加载

```
用户滚动到底部（或打开后下拉）
  → fetchAfter(chatId, lastSeq, 20)
  → RangeSet.addRange(lastSeq+1, newEnd)
  → Q_EMIT messagesInserted(chatId, msgs, InsertHint::Bottom)
  → ChatWidget 直接追加（无需锚点修正）
```

### jumpTo — 跳转定位

```
引用消息跳转 / 搜索结果跳转
  → fetchAround(chatId, targetSeq, 50)
  → RangeSet 重置为 [{targetSeq-50, targetSeq+50}]
  → Q_EMIT messagesInserted(chatId, msgs, InsertHint::Replace)
  → ChatWidget 清空列表，填充新数据，滚动到 targetSeq
```

## 完整数据流

```
┌─ 打开聊天 ──────────────────────────────────────────┐
│  fetchLatest(chatId, 20)                            │
│  fetchUpdated(chatId, maxVersion)                   │
│  messagesInserted(InsertHint::Replace) → scrollBottom│
└─────────────────────────────────────────────────────┘

┌─ 向上滚动触顶 ──────────────────────────────────────┐
│  fetchBefore(chatId, firstSeq, 20)                  │
│  fetchUpdated(chatId, maxVersion)                   │
│  messagesInserted(InsertHint::Top) → 锚点修正        │
└─────────────────────────────────────────────────────┘

┌─ 向下滚动触底 ──────────────────────────────────────┐
│  fetchAfter(chatId, lastSeq, 20)                    │
│  messagesInserted(InsertHint::Bottom) → append       │
└─────────────────────────────────────────────────────┘

┌─ 在线实时推送 ──────────────────────────────────────┐
│  onMessageStored(chatId)                            │
│    → fetchAfter(chatId, lastSeq, 50)                │
│    → messagesInserted(Bottom) → 在底则 scrollBottom  │
│                                                     │
│  onMessageUpdated(chatId, msgId)                    │
│    → 拉取最新版本 → messageUpdated → ChatWidget upsert│
└─────────────────────────────────────────────────────┘

┌─ 发送消息 ──────────────────────────────────────────┐
│  ChatService.sendMessage()                          │
│    → 服务端存储 → 推送 onMessageStored               │
│    → 走实时推送流程                                   │
└─────────────────────────────────────────────────────┘
```

## 设计要点

1. **双向触发**：同时检测触顶和触底，`isLoading` 防并发
2. **fetchUpdated 只管已缓存区间**：`RangeSet` 内的消息才检查更新
3. **版本号无冲突**：全局单调整数，替代时间戳
4. **锚点修正**：`InsertHint::Top` 时保存第一条消息的屏幕 Y 坐标，插入后修正滚动值
5. **upsert 语义**：`fetchUpdated` 返回的消息直接 upsert，保证幂等
