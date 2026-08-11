#pragma once

#include "di/lifecycle.hpp"
#include "events/event_listener.hpp"
#include "events/event_types.hpp"
#include "events/i_event_bus.hpp"
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::events {

/**
 * @brief Event bus for decoupled component communication.
 *
 * Similar to Spring's ApplicationEventPublisher, the EventBus allows
 * services to publish events and subscribe to events without direct
 * dependencies on each other.
 *
 * The event bus supports both synchronous and asynchronous event publishing:
 * - Publish(): Immediately invokes all listeners (useful for critical events)
 * - PublishAsync(): Queues event for next ProcessQueue() call (useful for cross-thread events)
 *
 * Example usage:
 * @code
 * EventBus eventBus;
 *
 * // Subscribe to specific event type
 * eventBus.Subscribe(EventType::KeyPressed, [](const Event& event) {
 *     auto keyEvent = event.GetData<KeyEvent>();
 *     std::cout << "Key: " << keyEvent.key << std::endl;
 * });
 *
 * // Publish event synchronously
 * KeyEvent data{SDLK_SPACE, SDL_SCANCODE_SPACE, SDL_KMOD_NONE, false};
 * eventBus.Publish(Event{EventType::KeyPressed, 0.0, data});
 *
 * // Or publish asynchronously (queued)
 * eventBus.PublishAsync(Event{EventType::KeyPressed, 0.0, data});
 * eventBus.ProcessQueue();  // Call once per frame
 * @endcode
 */
class EventBus : public IEventBus, public di::IInitializable, public di::IShutdownable {
public:
    EventBus() = default;
    ~EventBus() = default;

    // Non-copyable, non-movable
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    /**
     * @brief Subscribe to a specific event type.
     *
     * The listener will be called whenever an event of the specified type
     * is published (either via Publish() or PublishAsync()).
     *
     * @param type The event type to subscribe to
     * @param listener The callback function to invoke
     */
    void Subscribe(EventType type, EventListener listener) override;

    /**
     * @brief Subscribe to all event types.
     *
     * The listener will be called for every event published, regardless of type.
     * Useful for logging, debugging, or telemetry.
     *
     * @param listener The callback function to invoke for all events
     */
    void SubscribeAll(EventListener listener) override;

    /**
     * @brief Publish an event synchronously.
     *
     * Immediately invokes all listeners subscribed to this event type,
     * as well as all global listeners. This blocks until all listeners complete.
     *
     * Use this for critical events that must be processed immediately
     * (e.g., window resize, shutdown requests).
     *
     * @param event The event to publish
     */
    void Publish(const Event& event) override;

    /**
     * @brief Publish an event asynchronously.
     *
     * Queues the event for later processing. The event will be dispatched
     * to listeners when ProcessQueue() is called.
     *
     * Use this for non-critical events or when publishing from a different
     * thread (e.g., audio thread, network thread).
     *
     * @param event The event to publish
     */
    void PublishAsync(const Event& event) override;

    /**
     * @brief Process all queued asynchronous events.
     *
     * Dispatches all events queued via PublishAsync() to their subscribers.
     * This should be called once per frame in the main loop.
     *
     * Thread-safe: Can be called while other threads are calling PublishAsync().
     */
    void ProcessQueue() override;

    /**
     * @brief Remove all event listeners.
     *
     * Useful for testing or resetting the event bus state.
     */
    void ClearListeners() override;

    /**
     * @brief Get the number of listeners for a specific event type.
     *
     * @param type The event type to query
     * @return The number of listeners subscribed to this event type
     */
    size_t GetListenerCount(EventType type) const override;

    /**
     * @brief Get the number of global listeners.
     *
     * @return The number of listeners subscribed to all events
     */
    size_t GetGlobalListenerCount() const override;

    // IInitializable interface
    void Initialize() override {}

    // IShutdownable interface  
    void Shutdown() noexcept override {}

private:
    // Event type -> list of listeners
    std::unordered_map<EventType, std::vector<EventListener>> listeners_;

    // Listeners that receive all events
    std::vector<EventListener> globalListeners_;

    // Queue for asynchronous events
    std::queue<Event> eventQueue_;

    // Mutex to protect eventQueue_ (allows cross-thread PublishAsync)
    mutable std::mutex queueMutex_;

    // Helper to dispatch event to listeners
    void DispatchEvent(const Event& event);
};

}  // namespace sdl3cpp::events
