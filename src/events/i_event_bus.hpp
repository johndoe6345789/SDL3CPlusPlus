#pragma once

#include "events/event_listener.hpp"
#include "events/event_types.hpp"
#include <cstddef>

namespace sdl3cpp::events {

class IEventBus {
public:
    virtual ~IEventBus() = default;

    virtual void Subscribe(EventType type, EventListener listener) = 0;
    virtual void SubscribeAll(EventListener listener) = 0;
    virtual void Publish(const Event& event) = 0;
    virtual void PublishAsync(const Event& event) = 0;
    virtual void ProcessQueue() = 0;
    virtual void ClearListeners() = 0;
    virtual size_t GetListenerCount(EventType type) const = 0;
    virtual size_t GetGlobalListenerCount() const = 0;
};

}  // namespace sdl3cpp::events
