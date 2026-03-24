#pragma once

// 消息内容序列化工具（inline，供 LocalDatabase 和 MessageStore 共享）

#include <wechat/core/Message.h>

#include <nlohmann/json.hpp>
#include <string>

namespace wechat {
namespace cache {
namespace detail {

using json = nlohmann::json;

inline json metaToJson(const core::ResourceMeta& m) {
    return {{"size", m.size}, {"filename", m.filename}, {"extra", m.extra}};
}

inline core::ResourceMeta jsonToMeta(const json& j) {
    core::ResourceMeta m;
    m.size = j.value("size", std::size_t{0});
    m.filename = j.value("filename", "");
    if (j.contains("extra")) {
        m.extra = j["extra"].get<std::map<std::string, std::string>>();
    }
    return m;
}

inline json blockToJson(const core::ContentBlock& block) {
    return std::visit([](auto&& arg) -> json {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return {{"type", 0}};
        } else if constexpr (std::is_same_v<T, core::TextContent>) {
            return {{"type", 1}, {"text", arg.text}};
        } else if constexpr (std::is_same_v<T, core::ResourceContent>) {
            return {{"type", 2},
                    {"resourceId", arg.resourceId},
                    {"resType",    static_cast<int>(arg.type)},
                    {"resSubtype", static_cast<int>(arg.subtype)},
                    {"meta",       metaToJson(arg.meta)}};
        }
    }, block);
}

inline core::ContentBlock jsonToBlock(const json& j) {
    int type = j.value("type", 0);
    switch (type) {
    case 1:
        return core::TextContent{j.value("text", "")};
    case 2: {
        core::ResourceContent rc;
        rc.resourceId = j.value("resourceId", "");
        rc.type    = static_cast<core::ResourceType>(j.value("resType", 0));
        rc.subtype = static_cast<core::ResourceSubtype>(j.value("resSubtype", 0));
        if (j.contains("meta")) rc.meta = jsonToMeta(j["meta"]);
        return rc;
    }
    default:
        return std::monostate{};
    }
}

inline std::string serializeContent(const core::MessageContent& content) {
    json arr = json::array();
    for (const auto& block : content) {
        arr.push_back(blockToJson(block));
    }
    return arr.dump();
}

inline core::MessageContent deserializeContent(const std::string& str) {
    core::MessageContent content;
    auto arr = json::parse(str, nullptr, false);
    if (arr.is_discarded() || !arr.is_array()) return content;
    for (const auto& item : arr) {
        content.push_back(jsonToBlock(item));
    }
    return content;
}

} // namespace detail
} // namespace cache
} // namespace wechat
