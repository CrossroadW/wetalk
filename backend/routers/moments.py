"""
朋友圈相关路由处理
包括获取朋友圈、创建朋友圈
"""
import time
from core import get_db


async def handle_get_moments(current_token: str | None, msg_data: dict) -> dict:
    """处理获取朋友圈请求"""
    if not current_token:
        return {
            "type": "get_moments",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
        "type": "get_moments",
        "success": True,
        "data": {"moments": result}
    }


async def handle_create_moment(current_token: str | None, msg_data: dict) -> dict:
    """处理创建朋友圈请求"""
    if not current_token:
        return {
            "type": "create_moment",
            "success": False,
            "data": {"message": "未登录"}
        }

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

    return {
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
    }
