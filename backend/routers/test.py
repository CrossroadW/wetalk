"""测试相关的 API 端点"""
from fastapi import APIRouter
from core.database import reset_db

router = APIRouter(prefix="/api/test", tags=["test"])


@router.post("/reset-db")
async def reset_database():
    """重置数据库（仅用于测试）

    删除所有数据并重新初始化数据库表结构和测试数据。
    警告：此操作会清空所有数据！
    """
    reset_db()
    return {"success": True, "message": "Database reset successfully"}
