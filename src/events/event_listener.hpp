#pragma once

#include <functional>

namespace sdl3cpp::events {

// Forward declaration
struct Event;

/**
 * @brief Type alias for event listener callbacks.
 *
 * Event listeners are functions that receive an Event and process it.
 * Similar to Spring's @EventListener annotation, but as a function type.
 *
 * Example usage:
 * @code
 * EventListener listener = [](const Event& event) {
 *     if (event.type == EventType::KeyPressed) {
 *         auto keyEvent = event.GetData<KeyEvent>();
 *         std::cout << "Key pressed: " << keyEvent.key << std::endl;
 *     }
 * };
 * @endcode
 */
using EventListener = std::function<void(const Event&)>;

}  // namespace sdl3cpp::events
