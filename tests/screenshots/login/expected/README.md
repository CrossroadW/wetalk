# 登录界面截图说明

本目录存放登录界面各状态的预期截图，供自动化测试对比使用。

## 登录流程

### 无网络/服务器未连接

```
启动
  ↓
connecting — 尝试连接（显示连接中，每秒重试）
  ↓（连接成功）
qr_code 或 direct_login
```

### 无 token（首次登录）

```
启动
  ↓
qr_code          — 二维码界面（等待扫描）
  ↓（用户扫码）
qr_scanned       — 已扫描（等待手机端确认）
  ↓（手机确认）
qr_entering      — 正在进入（登录成功，跳转主界面）
  ↓
主界面
```

### 有 token（已登录过）

```
启动
  ↓
direct_login     — 直接登录界面（token 有效，显示进入按钮）
  ↓（点击进入）
qr_entering      — 正在进入
  ↓
主界面
```

---

## 截图说明

### connecting.png — 尝试连接

- 显示 WeTalk 标题
- 显示 "Loading..." 加载状态
- 提示文字：`Connecting to server...`（灰色）

**触发方式**：应用启动时立即显示此状态，无论服务器是否可用。每秒自动尝试连接，连接成功后切换到二维码或直接登录状态。

---

### qr_code.png — 二维码界面

- 显示 WeTalk 标题
- 显示二维码（由 `qr_login_init` 响应生成）
- 提示文字：`Scan QR code with WeChat mobile app`

**触发方式**：发送 `qr_login_init`，收到响应后 widget 渲染二维码。

---

### qr_scanned.png — 已扫描

- 二维码区域保持显示
- 提示文字变为绿色：`Scanned! Please confirm on your phone`

**触发方式**：后端推送 `qr_scanned` 消息（由 `POST /api/qr-confirm` 触发）。

---

### qr_entering.png — 正在进入

- 二维码区域显示 `Loading...`
- 提示文字：`Entering ChatFlow...`（蓝色）

**触发方式**：登录成功后（qr_confirmed 或 token 验证成功），跳转主界面前的过渡状态。

---

### direct_login.png — 直接登录界面

- 显示 `Welcome back, <username>!`
- 显示 `Enter WeChat` 按钮
- 不显示二维码

**触发方式**：发送 `verify_token`，token 有效时后端返回用户信息，widget 进入直接登录状态。

---

## 维护说明

- `expected/` 中的图片提交到 git，作为基准参考
- `output/` 中的图片由测试自动生成，被 git 忽略
- 如界面设计变更：
  1. 运行 `login_screen_test` 生成新截图
  2. 人工确认 `output/` 中的截图符合设计
  3. 将对应图片复制到 `expected/` 并提交
