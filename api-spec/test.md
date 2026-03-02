# 测试 API

> 仅用于测试环境的特殊 API 端点

## 概述

测试 API 提供了用于测试环境的特殊功能，主要用于自动化测试中的数据库管理。

**警告**: 这些 API 仅应在测试环境中使用，不应在生产环境中暴露。

## 端点列表

### POST /api/test/reset-db

重置数据库到初始状态。

**用途**:
- 在每个测试用例执行后清理数据
- 确保测试之间的隔离性
- 恢复到已知的初始状态

**请求**:
```http
POST /api/test/reset-db HTTP/1.1
Content-Type: application/json
```

**请求体**: 无

**响应**:
```json
{
  "success": true,
  "message": "Database reset successfully"
}
```

**行为**:
1. 删除现有数据库文件
2. 重新创建所有表结构
3. 插入测试用户数据（alice, bob, charlie, david, eve）
4. 插入测试好友关系（alice-bob, alice-charlie）

**测试数据**:

初始用户:
- alice (密码为空)
- bob (密码为空)
- charlie (密码为空)
- david (密码为空)
- eve (密码为空)

初始好友关系:
- alice ↔ bob
- alice ↔ charlie

**使用示例**:

Python (pytest):
```python
import httpx

async def test_example(ws_client):
    # 测试代码...
    pass
    # 测试完成后自动调用 reset-db (在 conftest.py 的 fixture 中)
```

C++ (GoogleTest):
```cpp
class MyTest : public ::testing::Test {
protected:
    void TearDown() override {
        // 测试完成后重置数据库
        QNetworkAccessManager mgr;
        QNetworkRequest req(QUrl("http://localhost:8000/api/test/reset-db"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QEventLoop loop;
        auto* reply = mgr.post(req, QByteArray{});
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        reply->deleteLater();
    }
};
```

## 最佳实践

### 测试隔离

每个测试用例应该在执行后重置数据库，确保:
1. 测试之间互不影响
2. 测试可以以任意顺序执行
3. 测试结果可重现

### 后端测试 (Python)

在 `conftest.py` 中配置 fixture:

```python
@pytest.fixture
async def ws_client(backend_server):
    import websockets
    import httpx

    uri = backend_server
    async with websockets.connect(uri) as websocket:
        yield websocket

    # 测试完成后重置数据库
    async with httpx.AsyncClient() as http_client:
        await http_client.post("http://127.0.0.1:8000/api/test/reset-db")
```

### 前端测试 (C++)

在测试类中添加 `TearDown` 方法:

```cpp
class LoginPresenterTest : public ::testing::Test {
protected:
    void TearDown() override {
        // 重置数据库
        QNetworkAccessManager mgr;
        QNetworkRequest req(QUrl("http://localhost:8000/api/test/reset-db"));
        auto* reply = mgr.post(req, QByteArray{});
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        reply->deleteLater();
    }
};
```

## 安全考虑

1. **仅测试环境**: 此 API 应仅在测试环境中启用
2. **无认证**: 当前实现不需要认证，因为仅用于本地测试
3. **生产环境**: 在生产环境中应完全禁用或移除此路由

## 相关文档

- [通用消息格式](common.md)
- [错误处理](errors.md)
