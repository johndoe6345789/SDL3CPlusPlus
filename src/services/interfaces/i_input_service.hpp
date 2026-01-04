#pragma once

#include <string>
#include <unordered_set>
#include <SDL3/SDL.h>

namespace sdl3cpp::services {

/**
 * @brief Input state snapshot for a single frame.
 */
struct InputState {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseWheelDeltaX = 0.0f;
    float mouseWheelDeltaY = 0.0f;
    std::unordered_set<SDL_Keycode> keysPressed;
    std::unordered_set<uint8_t> mouseButtonsPressed;
    std::string textInput;
};

/**
 * @brief Input handling service interface.
 *
 * Subscribes to input events from the event bus and maintains
 * the current input state for queries by other services.
 */
class IInputService {
public:
    virtual ~IInputService() = default;

    /**
     * @brief Process an SDL event and update input state.
     *
     * Called by the window service when events are polled.
     * Updates internal state and publishes events to the event bus.
     *
     * @param event The SDL event to process
     */
    virtual void ProcessEvent(const SDL_Event& event) = 0;

    /**
     * @brief Reset per-frame input state.
     *
     * Called at the beginning of each frame to clear transient state
     * like mouse wheel delta and text input.
     */
    virtual void ResetFrameState() = 0;

    /**
     * @brief Get the current input state.
     *
     * @return Reference to the current input state
     */
    virtual const InputState& GetState() const = 0;

    /**
     * @brief Check if a key is currently pressed.
     *
     * @param key The SDL keycode to check
     * @return true if the key is pressed, false otherwise
     */
    virtual bool IsKeyPressed(SDL_Keycode key) const = 0;

    /**
     * @brief Check if a mouse button is currently pressed.
     *
     * @param button The mouse button (SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, etc.)
     * @return true if the button is pressed, false otherwise
     */
    virtual bool IsMouseButtonPressed(uint8_t button) const = 0;

    /**
     * @brief Get the current mouse position.
     *
     * @return Pair of (x, y) coordinates in pixels
     */
    virtual std::pair<float, float> GetMousePosition() const = 0;
};

}  // namespace sdl3cpp::services
