#include "MockDataStore.h"

#include <algorithm>

namespace wechat::network {

MockDataStore::MockDataStore() : clock(1000000), idCounter(0) {}

int64_t MockDataStore::now() {
    std::lock_guard lock(mutex);
    return ++clock;
}

std::string MockDataStore::nextId(const std::string& prefix) {
    std::lock_guard lock(mutex);
    return prefix + std::to_string(++idCounter);
}

// ── 用户 / 认证 ──

std::string MockDataStore::addUser(const std::string& username,
                                   const std::string& password) {
    std::lock_guard lock(mutex);
    auto id = "u" + std::to_string(++idCounter);
    core::User user{id};
    usersByName[username] = UserRecord{user, password};
    userIdToName[id] = username;
    return id;
}

std::string MockDataStore::authenticate(const std::string& username,
                                        const std::string& password) {
    std::lock_guard lock(mutex);
    auto it = usersByName.find(username);
    if (it == usersByName.end() || it->second.password != password)
        return {};
    return it->second.user.id;
}

std::string MockDataStore::createToken(const std::string& userId) {
    std::lock_guard lock(mutex);
    auto token = "tok_" + std::to_string(++idCounter);
    tokens[token] = userId;
    return token;
}

std::string MockDataStore::resolveToken(const std::string& token) {
    std::lock_guard lock(mutex);
    auto it = tokens.find(token);
    return it != tokens.end() ? it->second : std::string{};
}

void MockDataStore::removeToken(const std::string& token) {
    std::lock_guard lock(mutex);
    tokens.erase(token);
}

core::User* MockDataStore::findUser(const std::string& userId) {
    std::lock_guard lock(mutex);
    auto nameIt = userIdToName.find(userId);
    if (nameIt == userIdToName.end()) return nullptr;
    auto it = usersByName.find(nameIt->second);
    if (it == usersByName.end()) return nullptr;
    return &it->second.user;
}

std::vector<core::User> MockDataStore::searchUsers(const std::string& keyword) {
    std::lock_guard lock(mutex);
    std::vector<core::User> result;
    for (auto& [name, record] : usersByName) {
        if (name.find(keyword) != std::string::npos ||
            record.user.id.find(keyword) != std::string::npos) {
            result.push_back(record.user);
        }
    }
    return result;
}

// ── 好友 ──

std::pair<std::string, std::string> MockDataStore::ordered(
    const std::string& a, const std::string& b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

void MockDataStore::addFriendship(const std::string& a, const std::string& b) {
    std::lock_guard lock(mutex);
    friendships.insert(ordered(a, b));
}

void MockDataStore::removeFriendship(const std::string& a,
                                     const std::string& b) {
    std::lock_guard lock(mutex);
    friendships.erase(ordered(a, b));
}

bool MockDataStore::areFriends(const std::string& a, const std::string& b) {
    std::lock_guard lock(mutex);
    return friendships.contains(ordered(a, b));
}

std::vector<std::string> MockDataStore::getFriendIds(
    const std::string& userId) {
    std::lock_guard lock(mutex);
    std::vector<std::string> result;
    for (auto& [a, b] : friendships) {
        if (a == userId) result.push_back(b);
        else if (b == userId) result.push_back(a);
    }
    return result;
}

// ── 群组 ──

core::Group& MockDataStore::createGroup(
    const std::string& ownerId,
    const std::vector<std::string>& memberIds) {
    std::lock_guard lock(mutex);
    auto id = "g" + std::to_string(++idCounter);
    core::Group group{id, ownerId, memberIds};
    auto [it, _] = groups.emplace(id, std::move(group));
    return it->second;
}

core::Group* MockDataStore::findGroup(const std::string& groupId) {
    std::lock_guard lock(mutex);
    auto it = groups.find(groupId);
    return it != groups.end() ? &it->second : nullptr;
}

void MockDataStore::removeGroup(const std::string& groupId) {
    std::lock_guard lock(mutex);
    groups.erase(groupId);
}

std::vector<core::Group> MockDataStore::getGroupsByUser(
    const std::string& userId) {
    std::lock_guard lock(mutex);
    std::vector<core::Group> result;
    for (auto& [_, g] : groups) {
        auto& m = g.memberIds;
        if (std::find(m.begin(), m.end(), userId) != m.end()) {
            result.push_back(g);
        }
    }
    return result;
}

// ── 消息 ──

core::Message& MockDataStore::addMessage(
    const std::string& senderId, const std::string& chatId,
    const std::string& replyTo, const core::MessageContent& content) {
    std::lock_guard lock(mutex);
    auto id = "m" + std::to_string(++idCounter);
    auto ts = ++clock;
    core::Message msg{id, senderId, chatId, replyTo, content,
                      ts, 0, false, 0, 0};
    auto [it, _] = messages.emplace(id, std::move(msg));
    chatMessages[chatId].push_back(id);
    return it->second;
}

core::Message* MockDataStore::findMessage(const std::string& messageId) {
    std::lock_guard lock(mutex);
    auto it = messages.find(messageId);
    return it != messages.end() ? &it->second : nullptr;
}

std::vector<core::Message> MockDataStore::getMessages(
    const std::string& chatId, int64_t sinceTs, int limit) {
    std::lock_guard lock(mutex);
    std::vector<core::Message> result;
    auto it = chatMessages.find(chatId);
    if (it == chatMessages.end()) return result;

    for (auto& msgId : it->second) {
        auto msgIt = messages.find(msgId);
        if (msgIt != messages.end() && msgIt->second.timestamp > sinceTs) {
            result.push_back(msgIt->second);
            if (static_cast<int>(result.size()) >= limit) break;
        }
    }
    return result;
}

// ── 朋友圈 ──

Moment& MockDataStore::addMoment(const std::string& authorId,
                                 const std::string& text,
                                 const std::vector<std::string>& imageIds) {
    std::lock_guard lock(mutex);
    auto id = "mo" + std::to_string(++idCounter);
    auto ts = ++clock;
    Moment moment{id, authorId, text, imageIds, ts, {}, {}};
    auto [it, _] = moments.emplace(id, std::move(moment));
    momentTimeline.insert(momentTimeline.begin(), id);
    return it->second;
}

Moment* MockDataStore::findMoment(const std::string& momentId) {
    std::lock_guard lock(mutex);
    auto it = moments.find(momentId);
    return it != moments.end() ? &it->second : nullptr;
}

std::vector<Moment> MockDataStore::getMoments(
    const std::set<std::string>& visibleUserIds,
    int64_t beforeTs, int limit) {
    std::lock_guard lock(mutex);
    std::vector<Moment> result;
    for (auto& id : momentTimeline) {
        auto it = moments.find(id);
        if (it == moments.end()) continue;
        auto& m = it->second;
        if (m.timestamp >= beforeTs) continue;
        if (!visibleUserIds.contains(m.authorId)) continue;
        result.push_back(m);
        if (static_cast<int>(result.size()) >= limit) break;
    }
    return result;
}

} // namespace wechat::network
