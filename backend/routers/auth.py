"""
认证相关路由处理
包括注册、登录、二维码登录、Token 验证、登出
"""
import secrets
import time
from fastapi import WebSocket
from managers import manager
from core import get_db


async def handle_register(websocket: WebSocket, msg_data: dict) -> dict:
    """
    处理用户注册请求

    Returns:
        response_data
    """
    username = msg_data.get("username", "").strip()
    password = msg_data.get("password", "").strip()

    if not username or not password:
        return {
            "type": "register",
            "success": False,
            "error": "用户名和密码不能为空"
        }

    with get_db() as conn:
        cursor = conn.cursor()

        # 检查用户名是否已存在
        cursor.execute("SELECT id FROM users WHERE username = ?", (username,))
        if cursor.fetchone():
            return {
                "type": "register",
                "success": False,
                "error": "用户名已存在"
            }

        # 创建新用户
        token = secrets.token_urlsafe(32)
        cursor.execute(
            "INSERT INTO users (username, password, token) VALUES (?, ?, ?)",
            (username, password, token)
        )
        conn.commit()

        user_id = cursor.lastrowid

        # 注册 WebSocket 连接
        await manager.connect(token, websocket)

        return {
            "type": "register",
            "success": True,
            "user": {
                "id": user_id,
                "username": username,
                "token": token
            }
        }


async def handle_login(websocket: WebSocket, msg_data: dict) -> dict:
    """
    处理用户登录请求

    Returns:
        response_data
    """
    username = msg_data.get("username", "").strip()
    password = msg_data.get("password", "").strip()

    if not username or not password:
        return {
            "type": "login",
            "success": False,
            "error": "用户名和密码不能为空"
        }

    with get_db() as conn:
        cursor = conn.cursor()

        # 验证用户名和密码
        cursor.execute(
            "SELECT id, username, password FROM users WHERE username = ?",
            (username,)
        )
        user = cursor.fetchone()

        if not user or user["password"] != password:
            return {
                "type": "login",
                "success": False,
                "error": "用户名或密码错误"
            }

        # 生成新 token
        token = secrets.token_urlsafe(32)
        cursor.execute(
            "UPDATE users SET token = ? WHERE id = ?",
            (token, user["id"])
        )
        conn.commit()

        # 注册 WebSocket 连接
        await manager.connect(token, websocket)

        return {
            "type": "login",
            "success": True,
            "user": {
                "id": user["id"],
                "username": user["username"],
                "token": token
            }
        }


async def handle_qr_login_init(websocket: WebSocket, msg_data: dict) -> tuple[str, dict]:
    """
    处理二维码登录初始化请求

    Returns:
        (session_id, response_data)
    """
    session_id = secrets.token_urlsafe(16)
    expires_at = int(time.time()) + 300  # 5分钟过期

    with get_db() as conn:
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO qr_sessions (session_id, status, created_at, expires_at) VALUES (?, 'pending', ?, ?)",
            (session_id, int(time.time()), expires_at)
        )
        conn.commit()

    # 注册为二维码监听者
    await manager.watch_qr(session_id, websocket)

    response = {
        "type": "qr_login_init",
        "success": True,
        "data": {
            "session_id": session_id,
            "qr_url": f"http://localhost:8000/qr-login?session={session_id}",
            "expires_at": expires_at
        }
    }

    return session_id, response


async def handle_verify_token(websocket: WebSocket, msg_data: dict) -> tuple[str | None, dict]:
    """
    处理 Token 验证请求

    Returns:
        (token, response_data)
    """
    token = msg_data.get("token")

    if not token:
        return None, {
            "type": "verify_token",
            "success": False,
            "data": {"message": "缺少 token"}
        }

    with get_db() as conn:
        cursor = conn.cursor()
        cursor.execute(
            "SELECT id, username FROM users WHERE token = ?",
            (token,)
        )
        user = cursor.fetchone()

        if user:
            await manager.connect(token, websocket)

            return token, {
                "type": "verify_token",
                "success": True,
                "data": {
                    "user_id": user["id"],
                    "username": user["username"]
                }
            }
        else:
            return None, {
                "type": "verify_token",
                "success": False,
                "data": {"message": "token 无效"}
            }


async def handle_logout(current_token: str | None) -> dict:
    """
    处理登出请求

    Returns:
        response_data
    """
    if current_token:
        with get_db() as conn:
            cursor = conn.cursor()
            cursor.execute("UPDATE users SET token = NULL WHERE token = ?", (current_token,))
            conn.commit()

        await manager.disconnect(current_token)

    return {
        "type": "logout",
        "success": True
    }
