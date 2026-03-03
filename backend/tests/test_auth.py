"""
认证相关测试
测试二维码登录和 Token 验证
"""
import pytest
import json
import asyncio


@pytest.mark.asyncio
async def test_qr_login_init(ws_client):
    """测试生成二维码"""
    # 发送请求
    await ws_client.send(json.dumps({
        "type": "qr_login_init",
        "data": {}
    }))

    # 接收响应
    response = await ws_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "qr_login_init"
    assert data["success"] is True
    assert "session_id" in data["data"]
    assert "qr_url" in data["data"]
    assert "expires_at" in data["data"]

    print(f"\n✅ 二维码生成成功: {data['data']['qr_url']}")


@pytest.mark.asyncio
async def test_qr_login_flow(ws_client):
    """
    测试完整的二维码登录流程（自动化）

    模拟用户扫码并确认登录
    """
    import httpx

    # 1. 请求生成二维码
    await ws_client.send(json.dumps({
        "type": "qr_login_init",
        "data": {}
    }))

    response = await ws_client.recv()
    data = json.loads(response)

    assert data["success"] is True
    session_id = data["data"]["session_id"]
    qr_url = data["data"]["qr_url"]

    print(f"\n🔗 二维码 URL: {qr_url}")

    # 2. 模拟用户扫码确认（自动调用 API）
    async with httpx.AsyncClient() as client:
        confirm_response = await client.post(
            "http://127.0.0.1:8000/api/qr-confirm",
            json={
                "session_id": session_id,
                "username": "alice"
            }
        )
        # 成功时返回 200
        assert confirm_response.status_code == 200
        confirm_data = confirm_response.json()
        assert confirm_data["success"] is True

    print("📱 模拟扫码确认成功")

    # 3. 等待服务器推送 qr_scanned
    response = await asyncio.wait_for(ws_client.recv(), timeout=5.0)
    data = json.loads(response)
    assert data["type"] == "qr_scanned"
    assert data["success"] is True

    # 4. 等待服务器推送 qr_confirmed
    response = await asyncio.wait_for(ws_client.recv(), timeout=5.0)
    data = json.loads(response)

    assert data["type"] == "qr_confirmed"
    assert data["success"] is True
    assert "token" in data["data"]
    assert "username" in data["data"]
    assert data["data"]["username"] == "alice"

    print(f"\n✅ 登录成功: {data['data']['username']}")
    print(f"🔑 Token: {data['data']['token']}")

    return data["data"]["token"]


@pytest.mark.asyncio
async def test_verify_token_invalid(ws_client):
    """测试无效 Token 验证"""
    # 发送无效 token
    await ws_client.send(json.dumps({
        "type": "verify_token",
        "data": {"token": "invalid_token_12345"}
    }))

    response = await ws_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "verify_token"
    assert data["success"] is False
    assert "message" in data["data"]

    print(f"\n✅ 无效 Token 正确拒绝: {data['data']['message']}")


@pytest.mark.asyncio
async def test_verify_token_valid(ws_client):
    """
    测试有效 Token 验证

    先通过登录流程获取有效 token，然后验证
    """
    import httpx

    # 1. 通过二维码登录获取 token
    await ws_client.send(json.dumps({
        "type": "qr_login_init",
        "data": {}
    }))

    response = await ws_client.recv()
    data = json.loads(response)
    session_id = data["data"]["session_id"]

    # 2. 模拟扫码确认
    async with httpx.AsyncClient() as client:
        await client.post(
            "http://127.0.0.1:8000/api/qr-confirm",
            json={
                "session_id": session_id,
                "username": "alice"
            }
        )

    # 3. 接收登录成功推送，获取 token
    # 先接收 qr_scanned
    response = await asyncio.wait_for(ws_client.recv(), timeout=2.0)
    data = json.loads(response)
    assert data["type"] == "qr_scanned"

    # 再接收 qr_confirmed
    response = await asyncio.wait_for(ws_client.recv(), timeout=2.0)
    data = json.loads(response)
    assert data["type"] == "qr_confirmed"
    token = data["data"]["token"]

    # 4. 验证 token
    await ws_client.send(json.dumps({
        "type": "verify_token",
        "data": {"token": token}
    }))

    response = await ws_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "verify_token"
    assert data["success"] is True
    assert "user_id" in data["data"]
    assert "username" in data["data"]
    assert data["data"]["username"] == "alice"

    print(f"\n✅ Token 验证成功: {data['data']['username']}")


@pytest.mark.asyncio
async def test_qr_login_nonexistent_user(ws_client):
    """测试不存在的用户登录（应该失败）"""
    import httpx

    # 1. 请求生成二维码
    await ws_client.send(json.dumps({
        "type": "qr_login_init",
        "data": {}
    }))

    response = await ws_client.recv()
    data = json.loads(response)
    assert data["success"] is True
    session_id = data["data"]["session_id"]

    # 2. 尝试用不存在的用户名确认
    async with httpx.AsyncClient() as client:
        confirm_response = await client.post(
            "http://127.0.0.1:8000/api/qr-confirm",
            json={
                "session_id": session_id,
                "username": "nonexistent_user_12345"
            }
        )
        # 失败时返回 400
        assert confirm_response.status_code == 400
        confirm_data = confirm_response.json()
        assert confirm_data["success"] is False
        assert "用户不存在" in confirm_data["detail"]

    print("\n✅ 不存在的用户正确拒绝")

    # 3. 验证客户端收到错误推送
    response = await asyncio.wait_for(ws_client.recv(), timeout=2.0)
    data = json.loads(response)
    assert data["type"] == "qr_login_failed"
    assert data["success"] is False
    assert "用户不存在" in data["data"]["message"]

    print("✅ 客户端收到错误推送")


@pytest.mark.asyncio
async def test_qr_login_missing_params():
    """测试缺少参数（应该失败）"""
    import httpx

    async with httpx.AsyncClient() as client:
        # 缺少 username
        response = await client.post(
            "http://127.0.0.1:8000/api/qr-confirm",
            json={"session_id": "test123"}
        )
        assert response.status_code == 400
        data = response.json()
        assert data["success"] is False
        assert "缺少参数" in data["detail"]

        # 缺少 session_id
        response = await client.post(
            "http://127.0.0.1:8000/api/qr-confirm",
            json={"username": "alice"}
        )
        assert response.status_code == 400
        data = response.json()
        assert data["success"] is False
        assert "缺少参数" in data["detail"]

    print("\n✅ 缺少参数正确拒绝")


@pytest.mark.asyncio
async def test_qr_login_invalid_session():
    """测试无效的 session_id（应该失败）"""
    import httpx

    async with httpx.AsyncClient() as client:
        response = await client.post(
            "http://127.0.0.1:8000/api/qr-confirm",
            json={
                "session_id": "invalid_session_12345",
                "username": "alice"
            }
        )
        assert response.status_code == 400
        data = response.json()
        assert data["success"] is False
        assert "二维码已失效" in data["detail"]

    print("\n✅ 无效 session_id 正确拒绝")


@pytest.mark.asyncio
async def test_qr_login_all_valid_users(ws_client):
    """测试所有有效用户都能登录"""
    import httpx

    valid_users = ["alice", "bob", "charlie", "david", "eve"]

    for username in valid_users:
        # 1. 生成二维码
        await ws_client.send(json.dumps({
            "type": "qr_login_init",
            "data": {}
        }))

        response = await ws_client.recv()
        data = json.loads(response)
        session_id = data["data"]["session_id"]

        # 2. 确认登录
        async with httpx.AsyncClient() as client:
            confirm_response = await client.post(
                "http://127.0.0.1:8000/api/qr-confirm",
                json={
                    "session_id": session_id,
                    "username": username
                }
            )
            assert confirm_response.status_code == 200
            confirm_data = confirm_response.json()
            assert confirm_data["success"] is True

        # 3. 接收推送（先 qr_scanned，再 qr_confirmed）
        response = await asyncio.wait_for(ws_client.recv(), timeout=2.0)
        data = json.loads(response)
        assert data["type"] == "qr_scanned"

        response = await asyncio.wait_for(ws_client.recv(), timeout=2.0)
        data = json.loads(response)
        assert data["type"] == "qr_confirmed"
        assert data["data"]["username"] == username

        print(f"✅ {username} 登录成功")
