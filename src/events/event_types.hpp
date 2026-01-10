#pragma once

#include <any>
#include <chrono>
#include <cstdint>
#include <string>
#include <SDL3/SDL.h>

namespace sdl3cpp::events {

/**
 * @brief Event type enumeration.
 *
 * Defines all event types that can be published on the event bus.
 * Similar to Spring's ApplicationEvent hierarchy, but using an enum
 * with type-erased data instead of inheritance.
 */
enum class EventType {
    // Window events
    WindowResized,
    WindowClosed,
    WindowMinimized,
    WindowMaximized,
    WindowRestored,
    WindowFocusGained,
    WindowFocusLost,

    // Input events
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseWheel,
    MouseGrabChanged,
    TextInput,

    // Rendering events
    FrameBegin,
    FrameEnd,
    SwapchainRecreated,
    RenderError,

    // Audio events
    AudioPlayRequested,
    AudioStopped,
    AudioError,

    // Script events
    ScriptLoaded,
    ScriptError,
    SceneLoaded,

    // Physics events
    PhysicsStepComplete,
    CollisionDetected,

    // Application lifecycle events
    ApplicationStarted,
    ApplicationShutdown,
    ApplicationPaused,
    ApplicationResumed,
};

/**
 * @brief Base event structure.
 *
 * Contains event type, timestamp, and type-erased data payload.
 * Services publish events and subscribers retrieve typed data using GetData<T>().
 */
struct Event {
    EventType type;
    double timestamp;  // Seconds since application start
    std::any data;     // Type-erased payload

    /**
     * @brief Retrieve typed data from the event.
     *
     * @tparam T The expected data type
     * @return const T& Reference to the data
     * @throws std::bad_any_cast if data is not of type T
     */
    template<typename T>
    const T& GetData() const {
        return std::any_cast<const T&>(data);
    }

    /**
     * @brief Check if event contains data of a specific type.
     *
     * @tparam T The type to check for
     * @return true if data is of type T, false otherwise
     */
    template<typename T>
    bool HasData() const {
        return data.type() == typeid(T);
    }
};

// ============================================================================
// Event Data Structures
// ============================================================================

/**
 * @brief Window resize event data.
 */
struct WindowResizedEvent {
    uint32_t width;
    uint32_t height;
};

/**
 * @brief Key press/release event data.
 */
struct KeyEvent {
    SDL_Keycode key;
    SDL_Scancode scancode;
    SDL_Keymod modifiers;
    bool repeat;
};

/**
 * @brief Mouse movement event data.
 */
struct MouseMovedEvent {
    float x;
    float y;
    float deltaX;
    float deltaY;
};

/**
 * @brief Mouse button press/release event data.
 */
struct MouseButtonEvent {
    uint8_t button;
    uint8_t clicks;
    float x;
    float y;
};

/**
 * @brief Mouse wheel event data.
 */
struct MouseWheelEvent {
    float deltaX;
    float deltaY;
    bool flipped;
};

/**
 * @brief Mouse grab state change event data.
 */
struct MouseGrabEvent {
    bool grabbed;
};

/**
 * @brief Text input event data.
 */
struct TextInputEvent {
    std::string text;
};

/**
 * @brief Frame timing event data.
 */
struct FrameEvent {
    uint64_t frameNumber;
    double deltaTime;  // Seconds since last frame
    double totalTime;  // Seconds since application start
};

/**
 * @brief Swapchain recreation event data.
 */
struct SwapchainRecreatedEvent {
    uint32_t newWidth;
    uint32_t newHeight;
};

/**
 * @brief Error event data.
 */
struct ErrorEvent {
    std::string message;
    std::string component;
};

/**
 * @brief Audio playback event data.
 */
struct AudioPlayEvent {
    std::string filePath;
    bool loop;
    bool background;  // true for music, false for sound effects
};

/**
 * @brief Collision detection event data.
 */
struct CollisionEvent {
    std::string objectA;
    std::string objectB;
    float impactForce;
};

}  // namespace sdl3cpp::events
