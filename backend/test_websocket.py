"""
WebSocket 客户端测试脚本
测试二维码登录和基本消息功能
"""
import asyncio
import websockets
import json

async def test_qr_login():
    """测试二维码登录流程"""
    uri = "ws://localhost:8000/ws"

    async with websockets.connect(uri) as websocket:
        print("✅ WebSocket 连接成功")

        # 1. 请求生成二维码
        print("\n📱 请求生成二维码...")
        await websocket.send(json.dumps({
            "type": "qr_login_init",
            "data": {}
        }))

        response = await websocket.recv()
        data = json.loads(response)
        print(f"✅ 收到响应: {data}")

        if data["success"]:
            session_id = data["data"]["session_id"]
            qr_url = data["data"]["qr_url"]
            print(f"\n🔗 二维码 URL: {qr_url}")
            print(f"📋 Session ID: {session_id}")
            print("\n请在浏览器中打开上述 URL 并输入用户名（如 alice）")
            print("等待扫码确认...")

            # 2. 等待扫码确认推送
            response = await websocket.recv()
            data = json.loads(response)
            print(f"\n✅ 收到推送: {data}")

            if data["type"] == "qr_confirmed" and data["success"]:
                token = data["data"]["token"]
                username = data["data"]["username"]
                print(f"\n🎉 登录成功!")
                print(f"👤 用户名: {username}")
                print(f"🔑 Token: {token}")
                return token

        return None

async def test_with_token(token: str):
    """使用 token 测试其他功能"""
    uri = "ws://localhost:8000/ws"

    async with websockets.connect(uri) as websocket:
        print("\n\n=== 测试 Token 验证 ===")

        # 1. 验证 token
        await websocket.send(json.dumps({
            "type": "verify_token",
            "data": {"token": token}
        }))

        response = await websocket.recv()
        data = json.loads(response)
        print(f"✅ Token 验证: {data}")

        if not data["success"]:
            print("❌ Token 无效")
            return

        # 2. 获取好友列表
        print("\n=== 测试获取好友列表 ===")
        await websocket.send(json.dumps({
            "type": "get_friends",
            "data": {}
        }))

        response = await websocket.recv()
        data = json.loads(response)
        print(f"✅ 好友列表: {data}")

        # 3. 获取群组列表
        print("\n=== 测试获取群组列表 ===")
        await websocket.send(json.dumps({
            "type": "get_groups",
            "data": {}
        }))

        response = await websocket.recv()
        data = json.loads(response)
        print(f"✅ 群组列表: {data}")

        # 4. 发送消息
        print("\n=== 测试发送消息 ===")
        await websocket.send(json.dumps({
            "type": "send_message",
            "data": {
                "chat_id": 1,
                "content_data": '{"blocks":[{"type":"text","text":"Hello from test!"}]}',
                "reply_to": 0
            }
        }))

        response = await websocket.recv()
        data = json.loads(response)
        print(f"✅ 发送消息: {data}")

        # 5. 获取消息列表
        print("\n=== 测试获取消息列表 ===")
        await websocket.send(json.dumps({
            "type": "get_messages",
            "data": {
                "chat_id": 1,
                "before_id": 0,
                "limit": 10
            }
        }))

        response = await websocket.recv()
        data = json.loads(response)
        print(f"✅ 消息列表: {data}")

async def main():
    """主测试流程"""
    print("=== WeTalk WebSocket API 测试 ===\n")
    print("请确保服务器已启动: uv run python main.py\n")

    try:
        # 测试二维码登录
        token = await test_qr_login()

        if token:
            # 使用 token 测试其他功能
            await test_with_token(token)
            print("\n\n✅ 所有测试完成!")
        else:
            print("\n❌ 登录失败")

    except Exception as e:
        print(f"\n❌ 测试失败: {e}")

if __name__ == "__main__":
    asyncio.run(main())
