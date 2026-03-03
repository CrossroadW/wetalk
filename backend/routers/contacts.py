"""
联系人相关路由处理
包括获取好友列表、添加好友
"""
from core import get_db


async def handle_get_friends(current_token: str | None) -> dict:
    """处理获取好友列表请求"""
    if not current_token:
        return {
            "type": "get_friends",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "get_friends",
        "success": True,
        "data": {"friends": friends}
    }


async def handle_add_friend(current_token: str | None, msg_data: dict) -> dict:
    """处理添加好友请求"""
    if not current_token:
        return {
            "type": "add_friend",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "add_friend",
        "success": True,
        "data": {}
    }
