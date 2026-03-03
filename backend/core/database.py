import sqlite3
from contextlib import contextmanager
from pathlib import Path

DB_PATH = Path(__file__).parent.parent / "wetalk.db"


def init_db():
    """初始化数据库表结构并添加测试数据"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # 用户表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL,
            token TEXT
        )
    """)

    # 群组表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS groups_ (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            owner_id INTEGER,
            updated_at INTEGER DEFAULT 0
        )
    """)

    # 群组成员
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS group_members (
            group_id INTEGER,
            user_id INTEGER,
            joined_at INTEGER NOT NULL,
            removed INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0,
            PRIMARY KEY (group_id, user_id)
        )
    """)

    # 好友关系
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS friendships (
            user_id_a INTEGER,
            user_id_b INTEGER,
            PRIMARY KEY (user_id_a, user_id_b)
        )
    """)

    # 消息表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER,
            chat_id INTEGER NOT NULL,
            reply_to INTEGER DEFAULT 0,
            content_data TEXT NOT NULL,
            revoked INTEGER DEFAULT 0,
            read_count INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0
        )
    """)

    # 朋友圈
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS moments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            author_id INTEGER NOT NULL,
            text TEXT NOT NULL DEFAULT '',
            timestamp INTEGER NOT NULL,
            updated_at INTEGER DEFAULT 0
        )
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS moment_images (
            moment_id INTEGER NOT NULL,
            image_id TEXT NOT NULL,
            sort_order INTEGER DEFAULT 0,
            PRIMARY KEY (moment_id, image_id)
        )
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS moment_likes (
            moment_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY (moment_id, user_id)
        )
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS moment_comments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            moment_id INTEGER NOT NULL,
            author_id INTEGER NOT NULL,
            text TEXT NOT NULL,
            timestamp INTEGER NOT NULL
        )
    """)

    # 二维码登录会话表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS qr_sessions (
            session_id TEXT PRIMARY KEY,
            status TEXT NOT NULL DEFAULT 'pending',
            user_id INTEGER,
            created_at INTEGER NOT NULL,
            expires_at INTEGER NOT NULL
        )
    """)

    # 索引
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_group_members_user ON group_members(user_id)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_messages_chat ON messages(chat_id, id)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_messages_reply ON messages(reply_to)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_messages_updated ON messages(chat_id, updated_at)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_moments_author ON moments(author_id)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_moment_comments_moment ON moment_comments(moment_id)")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_qr_sessions_expires ON qr_sessions(expires_at)")

    # 添加测试用户数据
    test_users = [
        ("alice", "123"),
        ("bob", "123"),
        ("charlie", "123"),
        ("david", "123"),
        ("eve", "123"),
    ]

    for username, password in test_users:
        cursor.execute(
            "INSERT OR IGNORE INTO users (username, password) VALUES (?, ?)",
            (username, password)
        )

    # 添加好友关系
    cursor.execute("SELECT id, username FROM users")
    users = {row[1]: row[0] for row in cursor.fetchall()}

    if "alice" in users and "bob" in users:
        user_a, user_b = sorted([users["alice"], users["bob"]])
        cursor.execute(
            "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
            (user_a, user_b)
        )

    if "alice" in users and "charlie" in users:
        user_a, user_b = sorted([users["alice"], users["charlie"]])
        cursor.execute(
            "INSERT OR IGNORE INTO friendships (user_id_a, user_id_b) VALUES (?, ?)",
            (user_a, user_b)
        )

    conn.commit()
    conn.close()


def reset_db():
    """重置数据库（删除所有数据并重新初始化）

    用于测试环境，清空所有表数据并重新创建表结构和测试数据
    """
    # 删除数据库文件
    if DB_PATH.exists():
        DB_PATH.unlink()

    # 重新初始化
    init_db()


@contextmanager
def get_db():
    """数据库连接上下文管理器"""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    try:
        yield conn
    finally:
        conn.close()
