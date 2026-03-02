from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
import secrets
import time
import json
from typing import Dict, Set
from pathlib import Path

from database import init_db, get_db

app = FastAPI(title="WeTalk WebSocket API", version="2.0.0")

# CORS 配置
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# WebSocket 连接管理
class ConnectionManager:
    def __init__(self):
        self.active_connections: Dict[str, WebSocket] = {}  # token -> websocket
        self.qr_watchers: Dict[str, WebSocket] = {}  # session_id -> websocket

    async def connect(self, token: str, websocket: WebSocket):
        await websocket.accept()
        self.active_connections[token] = websocket

    async def disconnect(self, token: str):
        if token in self.active_connections:
            del self.active_connections[token]

    async def watch_qr(self, session_id: str, websocket: WebSocket):
        self.qr_watchers[session_id] = websocket

    async def unwatch_qr(self, session_id: str):
        if session_id in self.qr_watchers:
            del self.qr_watchers[session_id]

    async def send_to_token(self, token: str, message: dict):
        if token in self.active_connections:
            await self.active_connections[token].send_json(message)

    async def send_to_qr_watcher(self, session_id: str, message: dict):
        if session_id in self.qr_watchers:
            await self.qr_watchers[session_id].send_json(message)

manager = ConnectionManager()


@app.on_event("startup")
def startup():
    init_db()
    print("✅ Database initialized")
    print("✅ WebSocket server ready")


# ==================== 二维码登录页面 ====================

@app.get("/qr-login")
async def qr_login_page(session: str):
    """手机扫码后显示的登录页面（仅需输入用户名）"""
    html_content = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>微信登录</title>
        <style>
            * {{ margin: 0; padding: 0; box-sizing: border-box; }}
            body {{
                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
                background: #ededed;
                display: flex;
                justify-content: center;
                align-items: center;
                min-height: 100vh;
                padding: 20px;
            }}
            .container {{
                background: white;
                border-radius: 8px;
                padding: 40px 30px;
                width: 100%;
                max-width: 400px;
                box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }}
            h1 {{
                font-size: 24px;
                color: #181818;
                margin-bottom: 10px;
                text-align: center;
            }}
            .subtitle {{
                color: #888;
                font-size: 14px;
                text-align: center;
                margin-bottom: 30px;
            }}
            .form-group {{
                margin-bottom: 20px;
            }}
            label {{
                display: block;
                color: #181818;
                font-size: 14px;
                margin-bottom: 8px;
            }}
            input {{
                width: 100%;
                padding: 12px 15px;
                border: 1px solid #d6d6d6;
                border-radius: 4px;
                font-size: 16px;
                outline: none;
            }}
            input:focus {{
                border-color: #07c160;
            }}
            button {{
                width: 100%;
                padding: 12px;
                background: #07c160;
                color: white;
                border: none;
                border-radius: 4px;
                font-size: 16px;
                cursor: pointer;
            }}
            button:hover {{
                background: #06ad56;
            }}
            button:disabled {{
                background: #ccc;
                cursor: not-allowed;
            }}
            .message {{
                margin-top: 15px;
                padding: 10px;
                border-radius: 4px;
                text-align: center;
                font-size: 14px;
            }}
            .error {{
                background: #fee;
                color: #c00;
            }}
            .success {{
                background: #efe;
                color: #060;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>登录微信</h1>
            <div class="subtitle">请输入您的用户名确认登录</div>
            <form id="loginForm">
                <div class="form-group">
                    <label for="username">用户名</label>
                    <input type="text" id="username" name="username" required autofocus>
                </div>
                <button type="submit" id="submitBtn">确认登录</button>
                <div id="message"></div>
            </form>
        </div>
        <script>
            const sessionId = "{session}";
            const form = document.getElementById("loginForm");
            const submitBtn = document.getElementById("submitBtn");
            const messageDiv = document.getElementById("message");

            form.addEventListener("submit", async (e) => {{
                e.preventDefault();
                const username = document.getElementById("username").value.trim();

                if (!username) {{
                    showMessage("请输入用户名", "error");
                    return;
                }}

                submitBtn.disabled = true;
                submitBtn.textContent = "登录中...";

                try {{
                    const response = await fetch("/api/qr-confirm", {{
                        method: "POST",
                        headers: {{ "Content-Type": "application/json" }},
                        body: JSON.stringify({{ session_id: sessionId, username: username }})
                    }});

                    const data = await response.json();

                    if (response.ok) {{
                        showMessage("登录成功！", "success");
                        setTimeout(() => {{
                            window.close();
                        }}, 1500);
                    }} else {{
                        showMessage(data.detail || "登录失败", "error");
                        submitBtn.disabled = false;
                        submitBtn.textContent = "确认登录";
                    }}
                }} catch (error) {{
                    showMessage("网络错误，请重试", "error");
                    submitBtn.disabled = false;
                    submitBtn.textContent = "确认登录";
                }}
            }});

            function showMessage(text, type) {{
                messageDiv.textContent = text;
                messageDiv.className = "message " + type;
            }}
        </script>
    </body>
    </html>
    """
    return HTMLResponse(content=html_content)


@app.post("/api/qr-confirm")
async def qr_confirm(data: dict):
    """手机端确认登录（仅需用户名）"""
    session_id = data.get("session_id")
    username = data.get("username")

    if not session_id or not username:
        return {"success": False, "detail": "缺少参数"}

    with get_db() as conn:
        cursor = conn.cursor()

        # 检查会话是否存在且未过期
        cursor.execute(
            "SELECT * FROM qr_sessions WHERE session_id = ? AND status = 'pending'",
            (session_id,)
        )
        session = cursor.fetchone()
        if not session:
            return {"success": False, "detail": "二维码已失效"}

        if session["expires_at"] < int(time.time()):
            return {"success": False, "detail": "二维码已过期"}

        # 查找用户（仅通过用户名）
        cursor.execute("SELECT id, username FROM users WHERE username = ?", (username,))
        user = cursor.fetchone()
        if not user:
            return {"success": False, "detail": "用户不存在"}

        # 生成 token
        token = secrets.token_urlsafe(32)
        cursor.execute("UPDATE users SET token = ? WHERE id = ?", (token, user["id"]))

        # 更新会话状态
        cursor.execute(
            "UPDATE qr_sessions SET status = 'confirmed', user_id = ? WHERE session_id = ?",
            (user["id"], session_id)
        )
        conn.commit()

        # 推送通知给桌面客户端
        await manager.send_to_qr_watcher(session_id, {
            "type": "qr_confirmed",
            "success": True,
            "data": {
                "user_id": user["id"],
                "username": user["username"],
                "token": token
            }
        })

        return {"success": True, "message": "登录成功"}


# ==================== WebSocket 主连接 ====================

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket 主连接端点"""
    await websocket.accept()
    current_token = None
    current_session = None

    try:
        while True:
            data = await websocket.receive_json()
            msg_type = data.get("type")
            msg_data = data.get("data", {})

            # ==================== 认证相关 ====================

            if msg_type == "qr_login_init":
                # 桌面客户端请求生成二维码
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
                current_session = session_id
                await manager.watch_qr(session_id, websocket)

                await websocket.send_json({
                    "type": "qr_login_init",
                    "success": True,
                    "data": {
                        "session_id": session_id,
                        "qr_url": f"http://localhost:8000/qr-login?session={session_id}",
                        "expires_at": expires_at
                    }
                })

            elif msg_type == "verify_token":
                # 验证 token 是否有效
                token = msg_data.get("token")
                if not token:
                    await websocket.send_json({
                        "type": "verify_token",
                        "success": False,
                        "data": {"message": "缺少 token"}
                    })
                    continue

                with get_db() as conn:
                    cursor = conn.cursor()
                    cursor.execute(
                        "SELECT id, username FROM users WHERE token = ?",
                        (token,)
                    )
                    user = cursor.fetchone()

                    if user:
                        current_token = token
                        await manager.connect(token, websocket)

                        await websocket.send_json({
                            "type": "verify_token",
                            "success": True,
                            "data": {
                                "user_id": user["id"],
                                "username": user["username"]
                            }
                        })
                    else:
                        await websocket.send_json({
                            "type": "verify_token",
                            "success": False,
                            "data": {"message": "token 无效"}
                        })

            elif msg_type == "logout":
                # 登出
                if current_token:
                    with get_db() as conn:
                        cursor = conn.cursor()
                        cursor.execute("UPDATE users SET token = NULL WHERE token = ?", (current_token,))
                        conn.commit()

                    await manager.disconnect(current_token)
                    current_token = None

                await websocket.send_json({
                    "type": "logout",
                    "success": True
                })

            # ==================== 聊天相关 ====================

            elif msg_type == "get_messages":
                # 获取消息列表
                if not current_token:
                    await websocket.send_json({
                        "type": "get_messages",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                chat_id = msg_data.get("chat_id")
                before_id = msg_data.get("before_id", 0)
                limit = msg_data.get("limit", 50)

                with get_db() as conn:
                    cursor = conn.cursor()
                    if before_id > 0:
                        cursor.execute(
                            """SELECT * FROM messages
                               WHERE chat_id = ? AND id < ?
                               ORDER BY id DESC LIMIT ?""",
                            (chat_id, before_id, limit)
                        )
                    else:
                        cursor.execute(
                            """SELECT * FROM messages
                               WHERE chat_id = ?
                               ORDER BY id DESC LIMIT ?""",
                            (chat_id, limit)
                        )

                    rows = cursor.fetchall()
                    messages = [dict(row) for row in rows]

                await websocket.send_json({
                    "type": "get_messages",
                    "success": True,
                    "data": {"messages": messages}
                })

            elif msg_type == "send_message":
                # 发送消息
                if not current_token:
                    await websocket.send_json({
                        "type": "send_message",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                chat_id = msg_data.get("chat_id")
                content_data = msg_data.get("content_data")
                reply_to = msg_data.get("reply_to", 0)

                with get_db() as conn:
                    cursor = conn.cursor()

                    # 获取当前用户
                    cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
                    user = cursor.fetchone()

                    # 插入消息
                    cursor.execute(
                        """INSERT INTO messages (sender_id, chat_id, reply_to, content_data, updated_at)
                           VALUES (?, ?, ?, ?, ?)""",
                        (user["id"], chat_id, reply_to, content_data, int(time.time()))
                    )
                    conn.commit()
                    msg_id = cursor.lastrowid

                    # 返回消息
                    cursor.execute("SELECT * FROM messages WHERE id = ?", (msg_id,))
                    row = cursor.fetchone()
                    message = dict(row)

                await websocket.send_json({
                    "type": "send_message",
                    "success": True,
                    "data": {"message": message}
                })

                # TODO: 推送给群组其他成员

            elif msg_type == "edit_message":
                # 编辑消息
                if not current_token:
                    await websocket.send_json({
                        "type": "edit_message",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                msg_id = msg_data.get("msg_id")
                content_data = msg_data.get("content_data")

                with get_db() as conn:
                    cursor = conn.cursor()
                    cursor.execute(
                        "UPDATE messages SET content_data = ?, updated_at = ? WHERE id = ?",
                        (content_data, int(time.time()), msg_id)
                    )
                    conn.commit()

                await websocket.send_json({
                    "type": "edit_message",
                    "success": True
                })

            elif msg_type == "revoke_message":
                # 撤回消息
                if not current_token:
                    await websocket.send_json({
                        "type": "revoke_message",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                msg_id = msg_data.get("msg_id")

                with get_db() as conn:
                    cursor = conn.cursor()
                    cursor.execute(
                        "UPDATE messages SET revoked = 1, updated_at = ? WHERE id = ?",
                        (int(time.time()), msg_id)
                    )
                    conn.commit()

                await websocket.send_json({
                    "type": "revoke_message",
                    "success": True
                })

            # ==================== 联系人相关 ====================

            elif msg_type == "get_friends":
                # 获取好友列表
                if not current_token:
                    await websocket.send_json({
                        "type": "get_friends",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                with get_db() as conn:
                    cursor = conn.cursor()

                    cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
                    user = cursor.fetchone()

                    cursor.execute(
                        """SELECT u.id, u.username FROM users u
                           JOIN friendships f ON (f.user_id_b = u.id AND f.user_id_a = ?)
                           OR (f.user_id_a = u.id AND f.user_id_b = ?)""",
                        (user["id"], user["id"])
                    )
                    rows = cursor.fetchall()
                    friends = [dict(row) for row in rows]

                await websocket.send_json({
                    "type": "get_friends",
                    "success": True,
                    "data": {"friends": friends}
                })

            elif msg_type == "add_friend":
                # 添加好友
                if not current_token:
                    await websocket.send_json({
                        "type": "add_friend",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                friend_id = msg_data.get("friend_id")

                with get_db() as conn:
                    cursor = conn.cursor()

                    cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
                    user = cursor.fetchone()

                    user_a, user_b = sorted([user["id"], friend_id])
                    cursor.execute(
                        "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
                        (user_a, user_b)
                    )
                    conn.commit()

                await websocket.send_json({
                    "type": "add_friend",
                    "success": True
                })

            # ==================== 群组相关 ====================

            elif msg_type == "get_groups":
                # 获取群组列表
                if not current_token:
                    await websocket.send_json({
                        "type": "get_groups",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                with get_db() as conn:
                    cursor = conn.cursor()

                    cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
                    user = cursor.fetchone()

                    cursor.execute(
                        """SELECT DISTINCT g.id, g.owner_id FROM groups_ g
                           JOIN group_members gm ON g.id = gm.group_id
                           WHERE gm.user_id = ? AND gm.removed = 0""",
                        (user["id"],)
                    )
                    groups = cursor.fetchall()

                    result = []
                    for g in groups:
                        cursor.execute(
                            "SELECT user_id FROM group_members WHERE group_id = ? AND removed = 0",
                            (g["id"],)
                        )
                        members = [row["user_id"] for row in cursor.fetchall()]
                        result.append({
                            "id": g["id"],
                            "owner_id": g["owner_id"],
                            "member_ids": members
                        })

                await websocket.send_json({
                    "type": "get_groups",
                    "success": True,
                    "data": {"groups": result}
                })

            elif msg_type == "create_group":
                # 创建群组
                if not current_token:
                    await websocket.send_json({
                        "type": "create_group",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                member_ids = msg_data.get("member_ids", [])

                with get_db() as conn:
                    cursor = conn.cursor()

                    cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
                    user = cursor.fetchone()

                    owner_id = user["id"] if len(member_ids) > 2 else 0
                    cursor.execute(
                        "INSERT INTO groups_ (owner_id, updated_at) VALUES (?, ?)",
                        (owner_id, int(time.time()))
                    )
                    group_id = cursor.lastrowid

                    for member_id in member_ids:
                        cursor.execute(
                            """INSERT INTO group_members (group_id, user_id, joined_at, updated_at)
                               VALUES (?, ?, ?, ?)""",
                            (group_id, member_id, int(time.time()), int(time.time()))
                        )

                    conn.commit()

                await websocket.send_json({
                    "type": "create_group",
                    "success": True,
                    "data": {
                        "id": group_id,
                        "owner_id": owner_id,
                        "member_ids": member_ids
                    }
                })

            # ==================== 朋友圈相关 ====================

            elif msg_type == "get_moments":
                # 获取朋友圈
                if not current_token:
                    await websocket.send_json({
                        "type": "get_moments",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                limit = msg_data.get("limit", 20)

                with get_db() as conn:
                    cursor = conn.cursor()
                    cursor.execute(
                        "SELECT * FROM moments ORDER BY timestamp DESC LIMIT ?",
                        (limit,)
                    )
                    moments = cursor.fetchall()

                    result = []
                    for m in moments:
                        cursor.execute(
                            "SELECT image_id FROM moment_images WHERE moment_id = ? ORDER BY sort_order",
                            (m["id"],)
                        )
                        images = [row["image_id"] for row in cursor.fetchall()]

                        cursor.execute(
                            "SELECT user_id FROM moment_likes WHERE moment_id = ?",
                            (m["id"],)
                        )
                        likes = [row["user_id"] for row in cursor.fetchall()]

                        cursor.execute(
                            "SELECT * FROM moment_comments WHERE moment_id = ? ORDER BY timestamp",
                            (m["id"],)
                        )
                        comments = [dict(row) for row in cursor.fetchall()]

                        result.append({
                            "id": m["id"],
                            "author_id": m["author_id"],
                            "text": m["text"],
                            "image_ids": images,
                            "timestamp": m["timestamp"],
                            "liked_by": likes,
                            "comments": comments
                        })

                await websocket.send_json({
                    "type": "get_moments",
                    "success": True,
                    "data": {"moments": result}
                })

            elif msg_type == "create_moment":
                # 创建朋友圈
                if not current_token:
                    await websocket.send_json({
                        "type": "create_moment",
                        "success": False,
                        "data": {"message": "未登录"}
                    })
                    continue

                text = msg_data.get("text", "")
                image_ids = msg_data.get("image_ids", [])

                with get_db() as conn:
                    cursor = conn.cursor()

                    cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
                    user = cursor.fetchone()

                    timestamp = int(time.time())
                    cursor.execute(
                        "INSERT INTO moments (author_id, text, timestamp) VALUES (?, ?, ?)",
                        (user["id"], text, timestamp)
                    )
                    moment_id = cursor.lastrowid

                    for i, img_id in enumerate(image_ids):
                        cursor.execute(
                            "INSERT INTO moment_images (moment_id, image_id, sort_order) VALUES (?, ?, ?)",
                            (moment_id, img_id, i)
                        )

                    conn.commit()

                await websocket.send_json({
                    "type": "create_moment",
                    "success": True,
                    "data": {
                        "id": moment_id,
                        "author_id": user["id"],
                        "text": text,
                        "image_ids": image_ids,
                        "timestamp": timestamp,
                        "liked_by": [],
                        "comments": []
                    }
                })

            else:
                await websocket.send_json({
                    "type": "error",
                    "success": False,
                    "data": {"message": f"未知消息类型: {msg_type}"}
                })

    except WebSocketDisconnect:
        if current_token:
            await manager.disconnect(current_token)
        if current_session:
            await manager.unwatch_qr(current_session)


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
