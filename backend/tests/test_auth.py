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
        assert confirm_response.status_code == 200
        confirm_data = confirm_response.json()
        assert confirm_data["success"] is True

    print("📱 模拟扫码确认成功")

    # 3. 等待服务器推送 qr_confirmed
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

    注意：需要先通过 test_qr_login_flow 获取有效 token
    这里使用数据库中已存在的 token 进行测试
    """
    # 从数据库中获取一个有效的 token（假设 alice 已登录）
    # 实际测试中应该先执行登录流程获取 token
    import sqlite3
    from pathlib import Path

    db_path = Path(__file__).parent.parent / "wetalk.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("SELECT token FROM users WHERE username = 'alice' AND token IS NOT NULL LIMIT 1")
    row = cursor.fetchone()
    conn.close()

    if not row or not row[0]:
        pytest.skip("数据库中没有有效的 token，请先运行登录测试")

    token = row[0]

    # 发送验证请求
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

    print(f"\n✅ Token 验证成功: {data['data']['username']}")
