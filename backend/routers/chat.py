"""
聊天相关路由处理
包括消息发送、获取、编辑、撤回
"""
import time
from core import get_db


async def handle_get_messages(current_token: str | None, msg_data: dict) -> dict:
    """处理获取消息列表请求"""
    if not current_token:
        return {
            "type": "get_messages",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "get_messages",
        "success": True,
        "data": {"messages": messages}
    }


async def handle_send_message(current_token: str | None, msg_data: dict) -> dict:
    """处理发送消息请求"""
    if not current_token:
        return {
            "type": "send_message",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "send_message",
        "success": True,
        "data": {"message": message}
    }

    # TODO: 推送给群组其他成员


async def handle_edit_message(current_token: str | None, msg_data: dict) -> dict:
    """处理编辑消息请求"""
    if not current_token:
        return {
            "type": "edit_message",
            "success": False,
            "data": {"message": "未登录"}
        }

    msg_id = msg_data.get("msg_id")
    content_data = msg_data.get("content_data")

    with get_db() as conn:
        cursor = conn.cursor()
        cursor.execute(
            "UPDATE messages SET content_data = ?, updated_at = ? WHERE id = ?",
            (content_data, int(time.time()), msg_id)
        )
        conn.commit()

    return {
        "type": "edit_message",
        "success": True
    }


async def handle_revoke_message(current_token: str | None, msg_data: dict) -> dict:
    """处理撤回消息请求"""
    if not current_token:
        return {
            "type": "revoke_message",
            "success": False,
            "data": {"message": "未登录"}
        }

    msg_id = msg_data.get("msg_id")

    with get_db() as conn:
        cursor = conn.cursor()
        cursor.execute(
            "UPDATE messages SET revoked = 1, updated_at = ? WHERE id = ?",
            (int(time.time()), msg_id)
        )
        conn.commit()

    return {
        "type": "revoke_message",
        "success": True
    }
