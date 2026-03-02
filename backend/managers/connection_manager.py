"""
WebSocket 连接管理器
管理活跃的 WebSocket 连接和二维码登录会话
"""
from typing import Dict
from fastapi import WebSocket


class ConnectionManager:
    """WebSocket 连接管理器"""

    def __init__(self):
        self.active_connections: Dict[str, WebSocket] = {}  # token -> websocket
        self.qr_watchers: Dict[str, WebSocket] = {}  # session_id -> websocket

    async def connect(self, token: str, websocket: WebSocket):
        """注册已认证的连接"""
        self.active_connections[token] = websocket

    async def disconnect(self, token: str):
        """断开已认证的连接"""
        if token in self.active_connections:
            del self.active_connections[token]

    async def watch_qr(self, session_id: str, websocket: WebSocket):
        """注册二维码监听者"""
        self.qr_watchers[session_id] = websocket

    async def unwatch_qr(self, session_id: str):
        """取消二维码监听"""
        if session_id in self.qr_watchers:
            del self.qr_watchers[session_id]

    async def send_to_token(self, token: str, message: dict):
        """向指定 token 的连接发送消息"""
        if token in self.active_connections:
            await self.active_connections[token].send_json(message)

    async def send_to_qr_watcher(self, session_id: str, message: dict):
        """向二维码监听者发送消息"""
        if session_id in self.qr_watchers:
            await self.qr_watchers[session_id].send_json(message)


# 全局连接管理器实例
manager = ConnectionManager()
