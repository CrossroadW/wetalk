"""
pytest 配置文件
提供测试 fixtures，包括自动启动/停止后端服务（同进程方案）
"""
import pytest
import asyncio
import sys
from pathlib import Path
from typing import AsyncGenerator
import uvicorn
from threading import Thread
import time

# 添加 backend 目录到 Python 路径
backend_dir = Path(__file__).parent
sys.path.insert(0, str(backend_dir))


class UvicornTestServer:
    """测试用的 Uvicorn 服务器（在独立线程中运行）"""

    def __init__(self, app, host="127.0.0.1", port=8000):
        self.host = host
        self.port = port
        self.config = uvicorn.Config(
            app,
            host=host,
            port=port,
            log_level="error",
            loop="asyncio"
        )
        self.server = uvicorn.Server(self.config)
        self.thread = None

    def start(self):
        """在独立线程中启动服务器"""
        self.thread = Thread(target=self.server.run, daemon=True)
        self.thread.start()
        # 等待服务器启动
        time.sleep(1)

    def stop(self):
        """停止服务器"""
        if self.server:
            self.server.should_exit = True
        if self.thread:
            self.thread.join(timeout=5)


@pytest.fixture(scope="session")
def backend_server():
    """
    启动后端服务器（同进程，独立线程）

    scope="session" 表示整个测试会话只启动一次
    测试完成后自动关闭
    """
    from main import app
    from core import init_db

    # 初始化数据库
    init_db()

    # 启动服务器
    server = UvicornTestServer(app, host="127.0.0.1", port=8000)
    server.start()

    print("\n✅ 后端服务器已启动 (同进程)")

    yield "ws://127.0.0.1:8000/ws"

    # 测试完成后停止服务器
    server.stop()
    print("\n✅ 后端服务器已停止")


@pytest.fixture
async def ws_client(backend_server) -> AsyncGenerator:
    """
    提供 WebSocket 客户端连接

    每个测试函数都会创建新的连接
    """
    import websockets

    uri = backend_server
    async with websockets.connect(uri) as websocket:
        yield websocket
