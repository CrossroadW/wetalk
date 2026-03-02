"""
测试脚本：添加测试用户到数据库
"""
import sqlite3
from pathlib import Path

DB_PATH = Path(__file__).parent / "wetalk.db"

def add_test_users():
    """添加测试用户"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    test_users = [
        ("alice", ""),
        ("bob", ""),
        ("charlie", ""),
        ("david", ""),
        ("eve", ""),
    ]

    for username, password in test_users:
        try:
            cursor.execute(
                "INSERT INTO users (username, password) VALUES (?, ?)",
                (username, password)
            )
            print(f"✅ 添加用户: {username}")
        except sqlite3.IntegrityError:
            print(f"⚠️  用户已存在: {username}")

    conn.commit()

    # 添加一些好友关系
    cursor.execute("SELECT id, username FROM users")
    users = {row[1]: row[0] for row in cursor.fetchall()}

    if "alice" in users and "bob" in users:
        user_a, user_b = sorted([users["alice"], users["bob"]])
        cursor.execute(
            "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
            (user_a, user_b)
        )
        print("✅ 添加好友关系: alice <-> bob")

    if "alice" in users and "charlie" in users:
        user_a, user_b = sorted([users["alice"], users["charlie"]])
        cursor.execute(
            "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
            (user_a, user_b)
        )
        print("✅ 添加好友关系: alice <-> charlie")

    conn.commit()
    conn.close()
    print("\n✅ 测试数据初始化完成")

if __name__ == "__main__":
    add_test_users()
