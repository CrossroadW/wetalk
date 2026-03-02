"""
聊天相关测试
测试消息发送、获取、编辑、撤回
"""
import pytest
import json


@pytest.fixture
async def authenticated_client(ws_client):
    """提供已认证的 WebSocket 客户端"""
    # 获取有效 token
    import sqlite3
    from pathlib import Path

    db_path = Path(__file__).parent.parent / "wetalk.db"
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("SELECT token FROM users WHERE username = 'alice' AND token IS NOT NULL LIMIT 1")
    row = cursor.fetchone()
    conn.close()

    if not row or not row[0]:
        pytest.skip("需要先登录获取 token")

    token = row[0]

    # 验证 token
    await ws_client.send(json.dumps({
        "type": "verify_token",
        "data": {"token": token}
    }))

    response = await ws_client.recv()
    data = json.loads(response)

    if not data["success"]:
        pytest.skip("Token 验证失败")

    return ws_client


@pytest.mark.asyncio
async def test_send_message(authenticated_client):
    """测试发送消息"""
    # 发送消息
    await authenticated_client.send(json.dumps({
        "type": "send_message",
        "data": {
            "chat_id": 1,
            "content_data": '{"blocks":[{"type":"text","text":"Hello from pytest!"}]}',
            "reply_to": 0
        }
    }))

    response = await authenticated_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "send_message"
    assert data["success"] is True
    assert "message" in data["data"]
    assert data["data"]["message"]["chat_id"] == 1

    print(f"\n✅ 消息发送成功: ID={data['data']['message']['id']}")


@pytest.mark.asyncio
async def test_get_messages(authenticated_client):
    """测试获取消息列表"""
    # 获取消息
    await authenticated_client.send(json.dumps({
        "type": "get_messages",
        "data": {
            "chat_id": 1,
            "before_id": 0,
            "limit": 10
        }
    }))

    response = await authenticated_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "get_messages"
    assert data["success"] is True
    assert "messages" in data["data"]
    assert isinstance(data["data"]["messages"], list)

    print(f"\n✅ 获取消息成功: 共 {len(data['data']['messages'])} 条")


@pytest.mark.asyncio
async def test_edit_message(authenticated_client):
    """测试编辑消息"""
    # 先发送一条消息
    await authenticated_client.send(json.dumps({
        "type": "send_message",
        "data": {
            "chat_id": 1,
            "content_data": '{"blocks":[{"type":"text","text":"Original message"}]}',
            "reply_to": 0
        }
    }))

    response = await authenticated_client.recv()
    data = json.loads(response)
    msg_id = data["data"]["message"]["id"]

    # 编辑消息
    await authenticated_client.send(json.dumps({
        "type": "edit_message",
        "data": {
            "msg_id": msg_id,
            "content_data": '{"blocks":[{"type":"text","text":"Edited message"}]}'
        }
    }))

    response = await authenticated_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "edit_message"
    assert data["success"] is True

    print(f"\n✅ 消息编辑成功: ID={msg_id}")


@pytest.mark.asyncio
async def test_revoke_message(authenticated_client):
    """测试撤回消息"""
    # 先发送一条消息
    await authenticated_client.send(json.dumps({
        "type": "send_message",
        "data": {
            "chat_id": 1,
            "content_data": '{"blocks":[{"type":"text","text":"Message to revoke"}]}',
            "reply_to": 0
        }
    }))

    response = await authenticated_client.recv()
    data = json.loads(response)
    msg_id = data["data"]["message"]["id"]

    # 撤回消息
    await authenticated_client.send(json.dumps({
        "type": "revoke_message",
        "data": {"msg_id": msg_id}
    }))

    response = await authenticated_client.recv()
    data = json.loads(response)

    # 验证响应
    assert data["type"] == "revoke_message"
    assert data["success"] is True

    print(f"\n✅ 消息撤回成功: ID={msg_id}")
