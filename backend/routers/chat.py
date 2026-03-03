"""
聊天相关路由处理
包括消息发送、获取、编辑、撤回、同步
"""
import time
from core import get_db
from managers import manager


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

        # 获取群组成员（推送给其他在线成员）
        cursor.execute(
            """SELECT u.token FROM users u
               JOIN group_members gm ON gm.user_id = u.id
               WHERE gm.group_id = ? AND u.id != ? AND u.token IS NOT NULL""",
            (chat_id, user["id"])
        )
        member_tokens = [row["token"] for row in cursor.fetchall()]

    # 推送给群组其他成员
    for token in member_tokens:
        await manager.send_to_token(token, {
            "type": "new_message",
            "success": True,
            "data": {"message": message}
        })

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
        "success": True,
        "data": {}
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
        "success": True,
        "data": {}
    }


async def handle_fetch_after(current_token: str | None, msg_data: dict) -> dict:
    """处理向下同步请求（新消息）"""
    if not current_token:
        return {
            "type": "fetch_after",
            "success": False,
            "data": {"message": "未登录"}
        }

    chat_id = msg_data.get("chat_id")
    after_id = msg_data.get("after_id", 0)
    limit = msg_data.get("limit", 50)

    with get_db() as conn:
        cursor = conn.cursor()

        if after_id > 0:
            # 获取 id > after_id 的消息
            cursor.execute(
                "SELECT * FROM messages WHERE chat_id = ? AND id > ? ORDER BY id ASC LIMIT ?",
                (chat_id, after_id, limit + 1)
            )
        else:
            # 获取最新的 limit 条消息
            cursor.execute(
                "SELECT * FROM messages WHERE chat_id = ? ORDER BY id DESC LIMIT ?",
                (chat_id, limit + 1)
            )

        rows = cursor.fetchall()
        has_more = len(rows) > limit
        messages = [dict(row) for row in rows[:limit]]

        # after_id=0 时需要反转顺序（从降序变为升序）
        if after_id == 0:
            messages.reverse()

    return {
        "type": "fetch_after",
        "success": True,
        "data": {"messages": messages, "has_more": has_more}
    }


async def handle_fetch_before(current_token: str | None, msg_data: dict) -> dict:
    """处理向上翻页请求（历史消息）"""
    if not current_token:
        return {
            "type": "fetch_before",
            "success": False,
            "data": {"message": "未登录"}
        }

    chat_id = msg_data.get("chat_id")
    before_id = msg_data.get("before_id", 0)
    limit = msg_data.get("limit", 50)

    with get_db() as conn:
        cursor = conn.cursor()

        if before_id > 0:
            # 获取 id < before_id 的消息
            cursor.execute(
                "SELECT * FROM messages WHERE chat_id = ? AND id < ? ORDER BY id DESC LIMIT ?",
                (chat_id, before_id, limit + 1)
            )
        else:
            # 获取最早的 limit 条消息
            cursor.execute(
                "SELECT * FROM messages WHERE chat_id = ? ORDER BY id ASC LIMIT ?",
                (chat_id, limit + 1)
            )

        rows = cursor.fetchall()
        has_more = len(rows) > limit
        messages = [dict(row) for row in rows[:limit]]

        # before_id>0 时需要反转顺序（从降序变为升序）
        if before_id > 0:
            messages.reverse()

    return {
        "type": "fetch_before",
        "success": True,
        "data": {"messages": messages, "has_more": has_more}
    }


async def handle_fetch_updated(current_token: str | None, msg_data: dict) -> dict:
    """处理增量更新请求"""
    if not current_token:
        return {
            "type": "fetch_updated",
            "success": False,
            "data": {"message": "未登录"}
        }

    chat_id = msg_data.get("chat_id")
    start_id = msg_data.get("start_id", 0)
    end_id = msg_data.get("end_id", 0)
    updated_at = msg_data.get("updated_at", 0)
    limit = msg_data.get("limit", 50)

    with get_db() as conn:
        cursor = conn.cursor()

        if updated_at > 0:
            cursor.execute(
                """SELECT * FROM messages
                   WHERE chat_id = ? AND id >= ? AND id <= ? AND updated_at > ?
                   ORDER BY id ASC LIMIT ?""",
                (chat_id, start_id, end_id, updated_at, limit + 1)
            )
        else:
            cursor.execute(
                """SELECT * FROM messages
                   WHERE chat_id = ? AND id >= ? AND id <= ?
                   ORDER BY id ASC LIMIT ?""",
                (chat_id, start_id, end_id, limit + 1)
            )

        rows = cursor.fetchall()
        has_more = len(rows) > limit
        messages = [dict(row) for row in rows[:limit]]

    return {
        "type": "fetch_updated",
        "success": True,
        "data": {"messages": messages, "has_more": has_more}
    }


async def handle_mark_read(current_token: str | None, msg_data: dict) -> dict:
    """处理标记已读请求"""
    if not current_token:
        return {
            "type": "mark_read",
            "success": False,
            "data": {"message": "未登录"}
        }

    chat_id = msg_data.get("chat_id")
    last_message_id = msg_data.get("last_message_id")

    with get_db() as conn:
        cursor = conn.cursor()

        # 获取当前用户 ID
        cursor.execute("SELECT id FROM users WHERE token = ?", (current_token,))
        user = cursor.fetchone()

        if not user:
            return {
                "type": "mark_read",
                "success": False,
                "data": {"message": "token 无效"}
            }

        # 增加非自己发送的消息的 read_count
        cursor.execute(
            """UPDATE messages SET read_count = read_count + 1
               WHERE chat_id = ? AND id <= ? AND sender_id != ?""",
            (chat_id, last_message_id, user["id"])
        )
        conn.commit()

    return {
        "type": "mark_read",
        "success": True,
        "data": {}
    }
