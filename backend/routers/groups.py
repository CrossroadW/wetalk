"""
群组相关路由处理
包括获取群组列表、创建群组
"""
import time
from core import get_db


async def handle_get_groups(current_token: str | None) -> dict:
    """处理获取群组列表请求"""
    if not current_token:
        return {
            "type": "get_groups",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "get_groups",
        "success": True,
        "data": {"groups": result}
    }


async def handle_create_group(current_token: str | None, msg_data: dict) -> dict:
    """处理创建群组请求"""
    if not current_token:
        return {
            "type": "create_group",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "create_group",
        "success": True,
        "data": {
            "id": group_id,
            "owner_id": owner_id,
            "member_ids": member_ids
        }
    }
