# 重构方案：LazyLoader 消息懒加载

> 基于 ChatGPT 参考实现，将现有 `SyncCursor [start, end]` + `updatedAt` 方案升级为
> `RangeSet` + `version` + `LazyLoader` 三件套。

## 问题诊断

| 问题 | 当前设计 | 影响 |
|------|---------|------|
| 单区间无法处理跳转 | `SyncCursor {start, end}` | jumpTo 后产生空洞，需全量重载 |
| 时间戳冲突 | `fetchUpdated(updatedAt)` | 同毫秒内多条更新会丢失 |
| 分页语义模糊 | `fetch(after_id=0)` 表示"最新" | 特殊值语义难理解、难测试 |
| 向下触底无触发 | 仅处理 `onReachedTop` | 跳转后向下滚动无法加载新消息 |

---

## 新设计

### 1. 数据模型变更

**Message 新增两个字段（见 [cpp-types.md](./cpp-types.md)）：**

```
chatSeq  int32   per-chat 单调自增序号（每个聊天从 1 开始）
version  int64   全局单调递增版本号（编辑/撤回时更新）
```

`updatedAt` → 重命名为 `version`（语义不变，改用全局单调计数器替代时间戳）

### 2. RangeSet — 多区间缓存管理

替换 `SyncCursor {start, end}`，支持非连续区间（jumpTo 产生的空洞）：

```cpp
struct Range { int32_t start; int32_t end; };

class RangeSet {
public:
    void addRange(int32_t start, int32_t end); // 自动合并重叠区间
    bool contains(int32_t chatSeq) const;
    int32_t firstSeq() const; // 最小 start
    int32_t lastSeq() const;  // 最大 end
};
```

### 3. LazyLoader — 加载控制层

`ChatPresenter` 扩展三个操作（对应参考实现的三个方法）：

| 操作 | 触发时机 | 行为 |
|------|---------|------|
| `loadOlder()` | 滚动触顶 | fetch before `firstSeq`，前插，保持锚点 |
| `loadNewer()` | 滚动触底 | fetch after `lastSeq`，后追加 |
| `jumpTo(chatSeq)` | 引用跳转/搜索 | fetchAround，清空视图，滚到目标 |

加载期间设 `isLoading` 防并发。

### 4. Fetch API 变更

```
旧：fetch(after_id, limit)  / fetch(before_id, limit)  / fetchUpdated(updatedAt)
新：fetchLatest(chatId, limit)
    fetchBefore(chatId, chatSeq, limit)
    fetchAfter(chatId, chatSeq, limit)
    fetchAround(chatId, chatSeq, radius)
    fetchUpdated(chatId, maxVersion, limit)
```

### 5. Qt 信号变更

```
旧：messagesInserted(chatId, messages)
新：messagesInserted(chatId, messages, InsertHint)  // hint = Top | Bottom | Replace
    chatJumped(chatId, targetChatSeq)               // jumpTo 完成后定位
```

---

## 实现步骤

### Step 1 — 数据模型（core）

- `Message` 新增 `chatSeq`，`updatedAt` → `version`
- 更新 `include/wechat/core/Message.h`
- 更新 `docs/cpp-types.md`

### Step 2 — DB 表结构（network/storage）

- messages 表新增 `chat_seq INTEGER NOT NULL`，`updated_at` → `version`
- 新增索引 `(chat_id, chat_seq)` 和 `(chat_id, version)`
- 服务端写入时自动分配 chat_seq（触发器或应用层）
- 更新 `docs/data-models.md`

### Step 3 — ChatService 接口（network）

- 拆分 `fetch` 为 `fetchLatest` / `fetchBefore` / `fetchAfter` / `fetchAround`
- `fetchUpdated` 参数由 `updatedAt` 改为 `maxVersion`
- 更新 `include/wechat/network/ChatService.h`

### Step 4 — RangeSet（chat）

- 新建 `src/chat/RangeSet.h` / `RangeSet.cpp`
- 含合并逻辑 + `contains()` + `firstSeq()` + `lastSeq()`
- 单元测试 `src/chat/tests/unit/test_range_set.cpp`

### Step 5 — ChatPresenter 重构（chat）

- 每个聊天维护 `RangeSet` 替换 `SyncCursor`
- 实现 `loadOlder()` / `loadNewer()` / `jumpTo(chatSeq)`
- `isLoading` 防并发（对应参考实现）
- `messagesInserted` 携带 `InsertHint` enum
- 更新 `include/wechat/chat/ChatPresenter.h`

### Step 6 — ChatWidget 滚动检测（chat）

- 同时检测触顶 (`value <= threshold`) 和触底 (`value >= max - threshold`)
- 触顶 → `presenter.loadOlder()`
- 触底 → `presenter.loadNewer()`
- 处理 `InsertHint::Top` 时保存锚点坐标，插入后修正滚动条（防跳动）

### Step 7 — 后端同步（backend）

- `chat_seq` 自动分配：写入时 `SELECT MAX(chat_seq)+1 FROM messages WHERE chat_id=?`
- `version` 用全局序列：`SELECT MAX(version)+1 FROM messages`（或独立计数表）
- 更新对应的 WebSocket handler

---

## 不变的部分

- MVP 分层架构（Widget → Presenter → Service → WebSocket）
- `fetchUpdated` 触发时机（每次 fetch 完成后）
- 实时推送语义（`onMessageStored` / `onMessageUpdated`）
- Qt 信号槽在 Presenter↔Widget 层的使用
- Boost.Signals2 在 Service↔Presenter 层的使用
