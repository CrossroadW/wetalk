"""
服务器配置
"""
import socket
import os


def get_local_ip() -> str:
    """
    获取本机局域网 IP 地址

    Returns:
        本机局域网 IP，如果获取失败则返回 localhost
    """
    try:
        # 创建一个 UDP socket 连接到外部地址（不会真正发送数据）
        # 这样可以获取本机用于连接外网的网卡 IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "localhost"


# 服务器配置
SERVER_HOST = os.getenv("SERVER_HOST", "0.0.0.0")
SERVER_PORT = int(os.getenv("SERVER_PORT", "8000"))

# 自动获取本机 IP 用于生成二维码 URL
# 可以通过环境变量 QR_BASE_URL 覆盖
QR_BASE_URL = os.getenv("QR_BASE_URL", f"http://{get_local_ip()}:{SERVER_PORT}")
