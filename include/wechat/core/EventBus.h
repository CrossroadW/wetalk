#pragma once

#include <boost/signals2/connection.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <wechat/core/Event.h>

namespace wechat {
namespace core {

/// 事件总线
///
/// 用法:
///   EventBus bus;
///
///   // 订阅（收到所有事件，用 visit 分发）
///   auto conn = bus.subscribe([](const Event& e) {
///       std::visit(overloaded{
///           [](const MessageReceived& ev) { /* ... */ },
///           [](const auto&) {}
///       }, e);
///   });
///
///   // 发布
///   bus.publish(MessageReceived{msg});
///
///   // 断开
///   conn.disconnect();
///
///   // 或用 scoped_connection 自动管理生命周期
///   boost::signals2::scoped_connection sc = bus.subscribe(handler);
///
class EventBus {
public:
    EventBus();
    ~EventBus();

    EventBus(EventBus const &) = delete;
    EventBus &operator=(EventBus const &) = delete;

    /// 订阅事件
    /// @param handler 事件处理回调
    /// @return boost::signals2::connection，可用于断开或转为 scoped_connection
    boost::signals2::connection
    subscribe(std::function<void(Event const &)> handler);

    /// 发布事件，分发给所有订阅者
    void publish(Event const &event);

    /// 获取当前订阅者数量
    [[nodiscard]] std::size_t subscriberCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace core
} // namespace wechat
