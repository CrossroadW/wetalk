# Cache 层架构设计

> 解决 network 与 cache 职责混乱的问题，引入 `MessageCache` 作为统一的数据访问层。

## 当前问题

```
ChatPresenter
  ├── 直接调用 ChatService.sendMessage()           ← 网络请求
  ├── 直接调用 ChatService.fetchAfter/Before()     ← 网络请求
  ├── 自己管理 SyncCursor {start, end}             ← 状态管理职责错放
  └── 订阅 ChatService.messageStored() 后再 fetch  ← 冗余网络请求

WsChatService::fetchBefore()
  └── ws.send() → QEventLoop 阻塞等响应           ← 卡 UI 线程
                                                   ← 响应无法与请求匹配（全双工流无 req_id）
```

| 问题 | 根因 |
|------|------|
| Presenter 直接操作网络 | 无 cache 层，每次操作都是 I/O |
| QEventLoop 阻塞主线程 | WebSocket 流式响应无 req_id，只能等待 |
| 发送后需额外 fetch | sendMessage 不等于收到消息，缺乏统一推送入口 |
| SyncCursor 在 Presenter 里 | 状态管理职责错放 |
| 无本地 SQLite 存储 | 切换聊天/重启需重新拉取 |

---

## 架构

```
┌─────────────────────────────────────────────────────────┐
│  UI：ChatWidget / SessionListWidget                      │
│           ↕ Qt signals (InsertHint)                     │
├─────────────────────────────────────────────────────────┤
│  Presenter：ChatPresenter / SessionPresenter             │
│           ↕ void 命令 + Qt signals                      │
├─────────────────────────────────────────────────────────┤
│  MessageCache                                           │
│   ├─ MessageStore (SQLite，同步，主线程可调)              │
│   ├─ RangeSet (已加载区间，内存状态)                      │
│   └─ 内部调用 ChatTransport（异步 QFuture）               │
├─────────────────────────────────────────────────────────┤
│  ChatTransport（纯网络，无状态，全异步）                   │
│   ├─ 请求：WsClient.request() → QFuture<Result<T>>      │
│   └─ 推送：WsClient.pushReceived() → Q_EMIT 信号        │
├─────────────────────────────────────────────────────────┤
│  WsClient                                               │
│   ├─ request(payload) → 分配 req_id，注册 QPromise       │
│   ├─ send(payload)    → fire-and-forget（无 req_id）     │
│   ├─ 收到响应：有 req_id → resolve QPromise              │
│   └─ 收到推送：无 req_id → Q_EMIT pushReceived(type,data)│
└─────────────────────────────────────────────────────────┘
              ↕  WebSocket (JSON + req_id)
           Backend
```

**核心原则**：
- 所有消息数据只从 `MessageCache` 流出，Presenter 不接触网络
- **本地 SQLite 是主存**：任何变化必须先落盘，写完再触发 UI 信号
- **全异步**：ChatTransport 所有方法返回 `QFuture<Result<T>>`，不阻塞任何线程
- 对标 TDLib：MessageCache 是唯一入口，上层不感知网络存在

---

## 异步设计（关键）

### 问题：WebSocket 是全双工流，如何匹配请求和响应？

**Telegram TDLib 的解法**：每个请求携带唯一 `req_id`，响应带回同一 `req_id`，内部用 map 维护 pending requests。

我们完全照搬这个模式：

### WsClient：区分 request-reply 和 push

```cpp
// include/wechat/network/WebSocketClient.h
class WebSocketClient : public QObject {
    Q_OBJECT
public:
    virtual void connectToServer(const QString& url) = 0;
    virtual bool isConnected() const = 0;

    /// 请求-响应：自动分配 req_id，返回 QFuture 等待匹配响应
    /// 超时或错误时 future 携带 error
    virtual QFuture<Result<QJsonObject>> request(QJsonObject payload) = 0;

    /// fire-and-forget：发送推送类消息（登录、心跳等），不等响应
    virtual void send(QJsonObject payload) = 0;

Q_SIGNALS:
    void connected();
    void disconnected();
    /// 服务端主动推送（无 req_id）：新消息、消息变更等
    void pushReceived(const QString& type, const QJsonObject& data);
    void error(const QString& message);
};
```

**WsClient 内部实现逻辑**：

```
request(payload):
  1. reqId = QString::number(nextReqId_++)
  2. payload["req_id"] = reqId
  3. pendingRequests_[reqId] = QPromise<Result<QJsonObject>>()
  4. socket_.sendTextMessage(...)
  5. 返回 promise.future()

onTextMessageReceived(msg):
  json = parse(msg)
  if json 含 "req_id":
    promise = pendingRequests_.take(json["req_id"])
    if json 含 "error": promise.addResult(unexpected(error))
    else:               promise.addResult(json)
    promise.finish()
  else:
    Q_EMIT pushReceived(json["type"], json)   ← 推送走这里
```

---

### ChatTransport：全异步接口

```cpp
// include/wechat/network/ChatTransport.h
class ChatTransport : public QObject {
    Q_OBJECT
public:
    // ── 写操作：fire-and-forget，结果经推送路径回来 ──
    virtual void sendMessage(const std::string& token, int64_t chatId,
                             int64_t replyTo,
                             const core::MessageContent& content) = 0;
    virtual void revokeMessage(const std::string& token, int64_t msgId) = 0;
    virtual void editMessage(const std::string& token, int64_t msgId,
                             const core::MessageContent& content) = 0;

    // ── 读操作：全异步，返回 QFuture ──
    virtual QFuture<Result<SyncResponse>> fetchLatest(
        const std::string& token, int64_t chatId, int limit) = 0;
    virtual QFuture<Result<SyncResponse>> fetchBefore(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int limit) = 0;
    virtual QFuture<Result<SyncResponse>> fetchAfter(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int limit) = 0;
    virtual QFuture<Result<SyncResponse>> fetchAround(
        const std::string& token, int64_t chatId,
        int32_t chatSeq, int radius) = 0;
    virtual QFuture<Result<SyncResponse>> fetchUpdated(
        const std::string& token, int64_t chatId,
        int64_t maxVersion, int limit) = 0;

Q_SIGNALS:
    /// 后端推送：新消息（自己发的 + 别人发的，后端统一推送）
    void messagePushed(int64_t chatId, core::Message msg);
    /// 后端推送：消息变更（撤回/编辑/已读数）
    void messageChangePushed(int64_t chatId, core::Message msg);
};
```

> `sendMessage` 是 void：发送只确认"送达服务器"，消息内容经后端推送 `messagePushed` 回来。
> 这样自己发的消息和别人发的消息走完全相同的接收路径。

---

### MessageCache：统一对外接口

```cpp
// include/wechat/cache/MessageCache.h
class MessageCache : public QObject {
    Q_OBJECT
public:
    explicit MessageCache(network::ChatTransport& transport,
                          QObject* parent = nullptr);
    void setSession(std::string token, int64_t userId);

    // ── 所有方法：void 命令，结果异步经信号返回 ──
    void loadLatest(int64_t chatId, int limit = 20);
    void loadOlder(int64_t chatId, int limit = 20);     // 触顶
    void loadNewer(int64_t chatId, int limit = 20);     // 触底
    void jumpTo(int64_t chatId, int32_t chatSeq, int radius = 50);

    void sendMessage(int64_t chatId, const core::MessageContent& content,
                     int64_t replyTo = 0);
    void revokeMessage(int64_t msgId);
    void editMessage(int64_t msgId, const core::MessageContent& content);

Q_SIGNALS:
    void messagesLoaded(int64_t chatId, std::vector<core::Message> msgs,
                        InsertHint hint, int32_t jumpTarget = -1);
    void messageChanged(int64_t chatId, core::Message msg);
    void loadFailed(int64_t chatId, QString reason);    // 网络失败，UI 可显示重试
};
```

---

### MessageCache 内部：async chain 写法

```cpp
void MessageCache::loadOlder(int64_t chatId, int limit) {
    auto& state = states_[chatId];
    if (state.isLoading) return;
    state.isLoading = true;

    auto firstSeq = state.ranges.firstSeq();

    transport_.fetchBefore(token_, chatId, firstSeq, limit)
        .then(this, [this, chatId](Result<SyncResponse> result) {
            // .then(this, ...) 在 MessageCache 所在线程（主线程）执行
            auto& state = states_[chatId];
            state.isLoading = false;

            if (!result) {
                Q_EMIT loadFailed(chatId, QString::fromStdString(result.error()));
                return;
            }

            auto& msgs = result->messages;
            if (msgs.empty()) return;

            store_.upsertMessages(msgs);                          // SQLite 同步写（主线程，<1ms）
            state.ranges.addRange(msgs.front().chatSeq, msgs.back().chatSeq);
            state.maxVersion = std::max(state.maxVersion,
                                        msgs.back().version);

            Q_EMIT messagesLoaded(chatId, msgs, InsertHint::Top); // 结果信号
        });
}
```

**为什么 SQLite 可以在主线程同步写？**
- 单条 upsert 批量 20 条消息 < 1ms，不影响帧率
- `.then(this, ...)` 保证在 `MessageCache` 线程（主线程）回调，无需加锁
- 如果未来消息量大，可以把 SQLite 放到 `QThreadPool`，外部接口不变

---

### 推送路径（最重要，自己发的消息也走这里）

```cpp
// MessageCache 构造函数中订阅推送信号
connect(&transport_, &ChatTransport::messagePushed,
        this, &MessageCache::onMessagePushed);
connect(&transport_, &ChatTransport::messageChangePushed,
        this, &MessageCache::onMessageChangePushed);

void MessageCache::onMessagePushed(int64_t chatId, core::Message msg) {
    store_.upsertMessage(msg);                              // 先落盘
    states_[chatId].ranges.addRange(msg.chatSeq, msg.chatSeq);
    Q_EMIT messagesLoaded(chatId, {msg}, InsertHint::Bottom);
}

void MessageCache::onMessageChangePushed(int64_t chatId, core::Message msg) {
    store_.upsertMessage(msg);                              // 先落盘
    Q_EMIT messageChanged(chatId, msg);
}
```

---

## 完整数据流

### 发送消息（自己发）

```
ChatWidget → ChatPresenter.sendMessage() → MessageCache.sendMessage()
  → ChatTransport.sendMessage()  [void, fire-and-forget]
  → WsClient.send(payload)       [无 req_id，不等响应]

        ↓ 后端存储，echo 推送

  WsClient.onMessage({"type":"new_message", ...})  [无 req_id]
  → Q_EMIT pushReceived("new_message", data)
  → WsChatTransport 解析 → Q_EMIT messagePushed(chatId, msg)
  → MessageCache.onMessagePushed()
      → SQLite upsert → Q_EMIT messagesLoaded(Bottom)
  → ChatPresenter → ChatWidget 追加+滚动
```

### 加载历史（触顶）

```
ChatWidget 触顶 → ChatPresenter.onReachedTop() → MessageCache.loadOlder()
  → ChatTransport.fetchBefore()
  → WsClient.request({"type":"fetch_before","req_id":"42",...})
                                    ← {"req_id":"42","messages":[...]}
  → QPromise resolved → QFuture ready
  → .then(this, lambda) 在主线程执行：
      SQLite upsert → RangeSet 扩展 → Q_EMIT messagesLoaded(Top)
  → ChatPresenter → ChatWidget 前插+锚点修正
```

### 网络失败

```
WsClient 超时 / 连接断开
  → QPromise.addResult(unexpected("timeout"))
  → .then() lambda 收到 error
  → Q_EMIT loadFailed(chatId, "timeout")
  → ChatPresenter → ChatWidget 显示"加载失败，点击重试"
```

---

## 模块划分与迁移

### 新增：`cache` 模块

```
src/cache/
├── MessageCache.h / .cpp     ← 统一数据入口
├── MessageStore.h / .cpp     ← SQLite 封装（内部，不导出）
├── RangeSet.h / .cpp         ← 多区间管理（内部，不导出）
└── tests/

include/wechat/cache/
├── MessageCache.h            ← 导出
└── InsertHint.h              ← InsertHint enum
```

### 变更：`network` 模块

| 原有 | 变更 | 说明 |
|------|------|------|
| `WebSocketClient::send()` + `messageReceived` | 拆为 `request()` + `send()` + `pushReceived` | req_id 匹配机制 |
| `ChatService`（sync fetch）| 重命名为 `ChatTransport`，接口改为 `QFuture` | 全异步 |
| `NetworkClient::chat()` | 不再对外暴露 ChatTransport | MessageCache 内部持有 |
| `WsChatService` | 重写为 `WsChatTransport` | 用 `ws.request()` 替代 QEventLoop |

### 变更：`chat` 模块

```
ChatPresenter.h/cpp   → 依赖改为 MessageCache（移除所有网络调用和 SyncCursor）
MessageListModel.h/cpp→ 新增，QAbstractListModel，持有当前视图窗口的消息列表
ChatWidget            → 持有 MessageListModel，setUniformItemSizes(true)
```

### 不变

- `WsClient` 连接管理逻辑（connectToServer、reconnect）
- `AuthService` — 登录/注册（流程简单，保持同步 QEventLoop 或独立 QFuture）
- `ContactService` / `GroupService` / `MomentService` — 后续按需迁移
- Qt 信号槽在 Presenter ↔ Widget 层的使用方式

---

## MessageListModel（虚拟列表层）

大聊天（10 万条消息）不能一次加载全部数据。在 `MessageCache` 和 `ChatWidget` 之间加一层 `QAbstractListModel`，只持有当前可见窗口的消息：

```cpp
// src/chat/MessageListModel.h
class MessageListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit MessageListModel(QObject* parent = nullptr);

    // ── QAbstractListModel 必须实现 ──
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // ── 懒加载支持（滚动到边界时触发）──
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // ── 从 MessageCache 信号接收数据 ──
    void onMessagesLoaded(int64_t chatId, std::vector<core::Message> msgs,
                          InsertHint hint, int32_t jumpTarget = -1);
    void onMessageChanged(int64_t chatId, core::Message msg);

private:
    std::vector<core::Message> items_;  // 仅当前视图窗口
    int64_t currentChatId_ = 0;
};
```

**连接关系**（在 `ChatPresenter` 或 `ChatPage` 中建立）：

```cpp
connect(cache, &MessageCache::messagesLoaded,
        model, &MessageListModel::onMessagesLoaded);
connect(cache, &MessageCache::messageChanged,
        model, &MessageListModel::onMessageChanged);

listView->setModel(model);
listView->setUniformItemSizes(true);  // 性能关键：统一行高时开启
```

**InsertHint 处理**：

| hint | model 操作 |
|------|-----------|
| `Bottom` | `beginInsertRows(end, end+n)` + 追加；如在底部则滚动到末尾 |
| `Top` | 保存锚点 Y，`beginInsertRows(0, n-1)` + 前插；修正滚动条防跳动 |
| `Replace` | `beginResetModel()` 清空 + 重填；`scrollTo(jumpTarget)` |

---

## 分页游标：chatSeq vs 全局 id

> **一个常见的设计陷阱**：用全局消息 `id` 做分页游标，RangeSet 会出现"虚假连续"。

```
chat_1 的消息 global id：[100, 103, 107, 112]   （中间夹着其他 chat 的消息）
如果 RangeSet 存 [100, 112]，contains(105) = true → 错误！以为已加载，实际没有
```

`chatSeq` 从 1 开始，每个 `chat_id` 独立计数，密集自增无跳跃：

```
chat_1 的消息 chatSeq：[1, 2, 3, 4]
RangeSet 存 [1, 4]，contains(3) = true → 正确
```

因此：
- `chatSeq` 用于：`RangeSet` 跟踪、`fetchBefore/fetchAfter` 的游标参数
- 全局 `id` 用于：消息唯一标识、`replyTo`、撤回/编辑操作

---

## SQLite 性能配置

`MessageStore` 初始化时开启 WAL 模式，主线程写 20 条消息 < 1ms：

```cpp
// MessageStore 初始化
db.exec("PRAGMA journal_mode=WAL");   // 写不阻塞读
db.exec("PRAGMA synchronous=NORMAL"); // WAL 下安全且更快
db.exec("PRAGMA cache_size=-8000");   // 8MB 页缓存
```

索引（见 data-models.md）：
```sql
CREATE INDEX idx_messages_chat_seq ON messages(chat_id, chat_seq);
CREATE INDEX idx_messages_version  ON messages(chat_id, version);
```

---

## 迁移步骤（有序）

1. `WsClient` 新增 `request()` → `QFuture<Result<QJsonObject>>`，req_id 机制
2. `RangeSet` 新建（`src/cache/RangeSet.h`）
3. `ChatTransport` 接口新建（`include/wechat/network/ChatTransport.h`）
4. `WsChatTransport` 实现（用 `ws.request()` 实现 fetch*，推送解析 emit 信号）
5. `MessageStore` 新建（SQLite CRUD + WAL 初始化，`src/cache/MessageStore.h`）
6. `MessageCache` 新建，串联 SQLite + ChatTransport + RangeSet
7. `MessageListModel` 新建（`QAbstractListModel`，处理 InsertHint 三种模式）
8. `ChatPresenter` 重构（依赖 MessageCache，删除 SyncCursor 和所有 fetch* 调用）
9. `ChatWidget` 改为持有 `MessageListModel`，`setUniformItemSizes(true)`
10. 后端适配 `chat_seq` + `version` + `req_id` 协议字段
