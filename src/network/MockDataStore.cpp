#include "MockDataStore.h"

#include <algorithm>
#include <chrono>

namespace wechat::network {

MockDataStore::MockDataStore() : idCounter(0) {}

int64_t MockDataStore::now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string MockDataStore::nextId(std::string const &prefix) {
    return prefix + std::to_string(++idCounter);
}

// ── 用户 / 认证 ──

std::string MockDataStore::addUser(std::string const &username,
                                   std::string const &password) {
    auto id = "u" + std::to_string(++idCounter);
    core::User user{id};
    usersByName[username] = UserRecord{user, password};
    userIdToName[id] = username;
    return id;
}

std::string MockDataStore::authenticate(std::string const &username,
                                        std::string const &password) {
    auto it = usersByName.find(username);
    if (it == usersByName.end() || it->second.password != password) {
        return {};
    }
    return it->second.user.id;
}

std::string MockDataStore::createToken(std::string const &userId) {
    auto token = "tok_" + std::to_string(++idCounter);
    tokens[token] = userId;
    return token;
}

std::string MockDataStore::resolveToken(std::string const &token) {
    auto it = tokens.find(token);
    return it != tokens.end() ? it->second : std::string{};
}

void MockDataStore::removeToken(std::string const &token) {
    tokens.erase(token);
}

core::User *MockDataStore::findUser(std::string const &userId) {
    auto nameIt = userIdToName.find(userId);
    if (nameIt == userIdToName.end()) {
        return nullptr;
    }
    auto it = usersByName.find(nameIt->second);
    if (it == usersByName.end()) {
        return nullptr;
    }
    return &it->second.user;
}

std::vector<core::User> MockDataStore::searchUsers(std::string const &keyword) {
    std::vector<core::User> result;
    for (auto &[name, record]: usersByName) {
        if (name.find(keyword) != std::string::npos ||
            record.user.id.find(keyword) != std::string::npos) {
            result.push_back(record.user);
        }
    }
    return result;
}

// ── 好友 ──

std::pair<std::string, std::string>
MockDataStore::ordered(std::string const &a, std::string const &b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

void MockDataStore::addFriendship(std::string const &a, std::string const &b) {
    friendships.insert(ordered(a, b));
}

void MockDataStore::removeFriendship(std::string const &a,
                                     std::string const &b) {
    friendships.erase(ordered(a, b));
}

bool MockDataStore::areFriends(std::string const &a, std::string const &b) {
    return friendships.contains(ordered(a, b));
}

std::vector<std::string>
MockDataStore::getFriendIds(std::string const &userId) {
    std::vector<std::string> result;
    for (auto &[a, b]: friendships) {
        if (a == userId) {
            result.push_back(b);
        } else if (b == userId) {
            result.push_back(a);
        }
    }
    return result;
}

// ── 群组 ──

core::Group &
MockDataStore::createGroup(std::string const &ownerId,
                           std::vector<std::string> const &memberIds) {
    auto id = "g" + std::to_string(++idCounter);
    core::Group group{id, ownerId, memberIds};
    auto [it, _] = groups.emplace(id, std::move(group));
    return it->second;
}

core::Group *MockDataStore::findGroup(std::string const &groupId) {
    auto it = groups.find(groupId);
    return it != groups.end() ? &it->second : nullptr;
}

void MockDataStore::removeGroup(std::string const &groupId) {
    groups.erase(groupId);
}

std::vector<core::Group>
MockDataStore::getGroupsByUser(std::string const &userId) {
    std::vector<core::Group> result;
    for (auto &[_, g]: groups) {
        auto &m = g.memberIds;
        if (std::find(m.begin(), m.end(), userId) != m.end()) {
            result.push_back(g);
        }
    }
    return result;
}

// ── 消息 ──

core::Message &MockDataStore::addMessage(std::string const &senderId,
                                         std::string const &chatId,
                                         std::string const &replyTo,
                                         core::MessageContent const &content) {
    auto id = "m" + std::to_string(++idCounter);
    auto ts = now();
    core::Message msg{id, senderId, chatId, replyTo, content,
                      ts, 0,        false,  0,       0};
    auto [it, _] = messages.emplace(id, std::move(msg));
    chatMessages[chatId].push_back(id);
    return it->second;
}

core::Message *MockDataStore::findMessage(std::string const &messageId) {
    auto it = messages.find(messageId);
    return it != messages.end() ? &it->second : nullptr;
}

std::vector<core::Message> MockDataStore::getMessages(std::string const &chatId,
                                                      int64_t sinceTs,
                                                      int limit) {
    std::vector<core::Message> result;
    auto it = chatMessages.find(chatId);
    if (it == chatMessages.end()) {
        return result;
    }

    for (auto &msgId: it->second) {
        auto msgIt = messages.find(msgId);
        if (msgIt != messages.end() && msgIt->second.timestamp > sinceTs) {
            result.push_back(msgIt->second);
            if (static_cast<int>(result.size()) >= limit) {
                break;
            }
        }
    }
    return result;
}

// ── 朋友圈 ──

Moment &MockDataStore::addMoment(std::string const &authorId,
                                 std::string const &text,
                                 std::vector<std::string> const &imageIds) {
    auto id = "mo" + std::to_string(++idCounter);
    auto ts = now();
    Moment moment{id, authorId, text, imageIds, ts, {}, {}};
    auto [it, _] = moments.emplace(id, std::move(moment));
    momentTimeline.insert(momentTimeline.begin(), id);
    return it->second;
}

Moment *MockDataStore::findMoment(std::string const &momentId) {
    auto it = moments.find(momentId);
    return it != moments.end() ? &it->second : nullptr;
}

std::vector<Moment>
MockDataStore::getMoments(std::set<std::string> const &visibleUserIds,
                          int64_t beforeTs, int limit) {
    std::vector<Moment> result;
    for (auto &id: momentTimeline) {
        auto it = moments.find(id);
        if (it == moments.end()) {
            continue;
        }
        auto &m = it->second;
        if (m.timestamp >= beforeTs) {
            continue;
        }
        if (!visibleUserIds.contains(m.authorId)) {
            continue;
        }
        result.push_back(m);
        if (static_cast<int>(result.size()) >= limit) {
            break;
        }
    }
    return result;
}

} // namespace wechat::network
