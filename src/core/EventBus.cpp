#include <boost/signals2/signal.hpp>
#include <wechat/core/EventBus.h>

namespace wechat {
namespace core {

struct EventBus::Impl {
    boost::signals2::signal<void(Event const &)> signal;
};

EventBus::EventBus() : impl_(std::make_unique<Impl>()) {}

EventBus::~EventBus() = default;

boost::signals2::connection
EventBus::subscribe(std::function<void(Event const &)> handler) {
    return impl_->signal.connect(std::move(handler));
}

void EventBus::publish(Event const &event) {
    impl_->signal(event);
}

std::size_t EventBus::subscriberCount() const {
    return impl_->signal.num_slots();
}

} // namespace core
} // namespace wechat
