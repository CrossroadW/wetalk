from pydantic import BaseModel
from typing import Optional, List


class UserRegister(BaseModel):
    username: str
    password: str


class UserLogin(BaseModel):
    username: str
    password: str


class User(BaseModel):
    id: int
    username: str
    token: Optional[str] = None


class MessageSend(BaseModel):
    chat_id: int
    content_data: str  # JSON string
    reply_to: int = 0


class Message(BaseModel):
    id: int
    sender_id: int
    chat_id: int
    reply_to: int
    content_data: str
    revoked: int
    read_count: int
    updated_at: int


class GroupCreate(BaseModel):
    member_ids: List[int]


class Group(BaseModel):
    id: int
    owner_id: int
    member_ids: List[int]


class MomentCreate(BaseModel):
    text: str
    image_ids: List[str] = []


class Moment(BaseModel):
    id: int
    author_id: int
    text: str
    image_ids: List[str]
    timestamp: int
    liked_by: List[int]
    comments: List[dict]
