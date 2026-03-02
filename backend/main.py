from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
import secrets
import time

from core import init_db, get_db
from managers import manager
from routers import auth, chat, contacts, groups, moments

app = FastAPI(title="WeTalk WebSocket API", version="2.0.0")

# CORS 配置
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


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
                current_session, response = await auth.handle_qr_login_init(websocket, msg_data)
                await websocket.send_json(response)

            elif msg_type == "verify_token":
                token, response = await auth.handle_verify_token(websocket, msg_data)
                if token:
                    current_token = token
                await websocket.send_json(response)

            elif msg_type == "logout":
                response = await auth.handle_logout(current_token)
                current_token = None
                await websocket.send_json(response)

            # ==================== 聊天相关 ====================

            elif msg_type == "get_messages":
                response = await chat.handle_get_messages(current_token, msg_data)
                await websocket.send_json(response)

            elif msg_type == "send_message":
                response = await chat.handle_send_message(current_token, msg_data)
                await websocket.send_json(response)

            elif msg_type == "edit_message":
                response = await chat.handle_edit_message(current_token, msg_data)
                await websocket.send_json(response)

            elif msg_type == "revoke_message":
                response = await chat.handle_revoke_message(current_token, msg_data)
                await websocket.send_json(response)

            # ==================== 联系人相关 ====================

            elif msg_type == "get_friends":
                response = await contacts.handle_get_friends(current_token)
                await websocket.send_json(response)

            elif msg_type == "add_friend":
                response = await contacts.handle_add_friend(current_token, msg_data)
                await websocket.send_json(response)

            # ==================== 群组相关 ====================

            elif msg_type == "get_groups":
                response = await groups.handle_get_groups(current_token)
                await websocket.send_json(response)

            elif msg_type == "create_group":
                response = await groups.handle_create_group(current_token, msg_data)
                await websocket.send_json(response)

            # ==================== 朋友圈相关 ====================

            elif msg_type == "get_moments":
                response = await moments.handle_get_moments(current_token, msg_data)
                await websocket.send_json(response)

            elif msg_type == "create_moment":
                response = await moments.handle_create_moment(current_token, msg_data)
                await websocket.send_json(response)

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
