"""
测试脚本：添加测试用户到数据库
"""
import sys
from pathlib import Path

# 添加 backend 目录到 Python 路径
backend_dir = Path(__file__).parent.parent
sys.path.insert(0, str(backend_dir))

from core import init_db, get_db


def add_test_users():
    """添加测试用户"""
    # 先初始化数据库表结构
    print("📦 初始化数据库...")
    init_db()
    print("✅ 数据库表结构已创建\n")

    test_users = [
        ("alice", ""),
        ("bob", ""),
        ("charlie", ""),
        ("david", ""),
        ("eve", ""),
    ]

    with get_db() as conn:
        cursor = conn.cursor()

        # 添加用户
        for username, password in test_users:
            try:
                cursor.execute(
                    "INSERT INTO users (username, password) VALUES (?, ?)",
                    (username, password)
                )
                print(f"✅ 添加用户: {username}")
            except Exception as e:
                if "UNIQUE constraint failed" in str(e):
                    print(f"⚠️  用户已存在: {username}")
                else:
                    print(f"❌ 添加用户失败 {username}: {e}")

        conn.commit()

        # 添加一些好友关系
        cursor.execute("SELECT id, username FROM users")
        users = {row["username"]: row["id"] for row in cursor.fetchall()}

        if "alice" in users and "bob" in users:
            user_a, user_b = sorted([users["alice"], users["bob"]])
            cursor.execute(
                "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
                (user_a, user_b)
            )
            if cursor.rowcount > 0:
                print("✅ 添加好友关系: alice <-> bob")
            else:
                print("⚠️  好友关系已存在: alice <-> bob")

        if "alice" in users and "charlie" in users:
            user_a, user_b = sorted([users["alice"], users["charlie"]])
            cursor.execute(
                "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
                (user_a, user_b)
            )
            if cursor.rowcount > 0:
                print("✅ 添加好友关系: alice <-> charlie")
            else:
                print("⚠️  好友关系已存在: alice <-> charlie")

        conn.commit()

    print("\n✅ 测试数据初始化完成")


if __name__ == "__main__":
    add_test_users()
