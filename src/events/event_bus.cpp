#include "events/event_bus.hpp"

namespace sdl3cpp::events {

void EventBus::Subscribe(EventType type, EventListener listener) {
    listeners_[type].push_back(std::move(listener));
}

void EventBus::SubscribeAll(EventListener listener) {
    globalListeners_.push_back(std::move(listener));
}

void EventBus::Publish(const Event& event) {
    DispatchEvent(event);
}

void EventBus::PublishAsync(const Event& event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    eventQueue_.push(event);
}

void EventBus::ProcessQueue() {
    // Lock to swap the queue (minimize lock time)
    std::queue<Event> localQueue;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        localQueue.swap(eventQueue_);
    }

    // Process all queued events without holding the lock
    while (!localQueue.empty()) {
        DispatchEvent(localQueue.front());
        localQueue.pop();
    }
}

void EventBus::ClearListeners() {
    listeners_.clear();
    globalListeners_.clear();
}

size_t EventBus::GetListenerCount(EventType type) const {
    auto it = listeners_.find(type);
    return it != listeners_.end() ? it->second.size() : 0;
}

size_t EventBus::GetGlobalListenerCount() const {
    return globalListeners_.size();
}

void EventBus::DispatchEvent(const Event& event) {
    // Dispatch to type-specific listeners
    auto it = listeners_.find(event.type);
    if (it != listeners_.end()) {
        for (const auto& listener : it->second) {
            listener(event);
        }
    }

    // Dispatch to global listeners
    for (const auto& listener : globalListeners_) {
        listener(event);
    }
}

}  // namespace sdl3cpp::events
