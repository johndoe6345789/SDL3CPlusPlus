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
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
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
 * Also handles GUI input processing for GUI workflows.
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
     * @brief Check if an action is currently pressed based on input bindings.
     *
     * @param action The action name to check
     * @return true if the action is pressed, false otherwise
     */
    virtual bool IsActionPressed(const std::string& action) const = 0;

    /**
     * @brief Get the current mouse position.
     *
     * @return Pair of (x, y) coordinates in pixels
     */
    virtual std::pair<float, float> GetMousePosition() const = 0;

    /**
     * @brief Set whether mouse input should be treated as relative motion.
     *
     * @param enabled true for relative mode, false for absolute
     */
    virtual void SetRelativeMouseMode(bool enabled) = 0;

    /**
     * @brief Check whether mouse input is treated as relative motion.
     *
     * @return true if relative mode is enabled, false otherwise
     */
    virtual bool IsRelativeMouseMode() const = 0;

    /**
     * @brief Update GUI input state for downstream consumers.
     */
    virtual void UpdateGuiInput() = 0;
};

}  // namespace sdl3cpp::services
