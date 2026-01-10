#pragma once

#include "services/interfaces/i_input_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "../../../events/i_event_bus.hpp"
#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief SDL3-based input service implementation.
 *
 * Subscribes to input events from the event bus and maintains
 * the current input state for queries. Also handles GUI input processing.
 */
class SdlInputService : public IInputService {
public:
    /**
     * @brief Construct with event bus dependency.
     *
     * Subscribes to input events automatically.
     *
     * @param eventBus Event bus to subscribe to
     */
    explicit SdlInputService(std::shared_ptr<events::IEventBus> eventBus,
                             std::shared_ptr<IConfigService> configService,
                             std::shared_ptr<ILogger> logger);
    ~SdlInputService() override;

    // IInputService interface
    void ProcessEvent(const SDL_Event& event) override;
    void ResetFrameState() override;
    const InputState& GetState() const override {
        if (logger_) {
            logger_->Trace("SdlInputService", "GetState");
        }
        return state_;
    }
    bool IsKeyPressed(SDL_Keycode key) const override;
    bool IsMouseButtonPressed(uint8_t button) const override;
    bool IsActionPressed(const std::string& action) const override;
    std::pair<float, float> GetMousePosition() const override;
    void SetRelativeMouseMode(bool enabled) override;
    bool IsRelativeMouseMode() const override;
    void UpdateGuiInput() override;

private:
    std::shared_ptr<events::IEventBus> eventBus_;
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    InputState state_;
    GuiInputSnapshot guiInputSnapshot_;
    SDL_Gamepad* gamepad_ = nullptr;
    bool windowFocused_ = true;
    bool mouseGrabbed_ = false;
    bool mouseGrabGatesLook_ = false;
    bool mouseRelativeMode_ = false;
    float guiCursorX_ = 0.0f;
    float guiCursorY_ = 0.0f;
    uint32_t guiWindowWidth_ = 0;
    uint32_t guiWindowHeight_ = 0;
    bool relativeCursorLogged_ = false;
    SDL_GamepadButton musicToggleButton_ = SDL_GAMEPAD_BUTTON_START;
    SDL_GamepadButton dpadUpButton_ = SDL_GAMEPAD_BUTTON_DPAD_UP;
    SDL_GamepadButton dpadDownButton_ = SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    SDL_GamepadButton dpadLeftButton_ = SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    SDL_GamepadButton dpadRightButton_ = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    SDL_GamepadAxis moveXAxis_ = SDL_GAMEPAD_AXIS_LEFTX;
    SDL_GamepadAxis moveYAxis_ = SDL_GAMEPAD_AXIS_LEFTY;
    SDL_GamepadAxis lookXAxis_ = SDL_GAMEPAD_AXIS_RIGHTX;
    SDL_GamepadAxis lookYAxis_ = SDL_GAMEPAD_AXIS_RIGHTY;
    std::unordered_map<SDL_GamepadButton, std::string> gamepadButtonActions_;
    std::unordered_map<SDL_GamepadAxis, std::string> gamepadAxisActions_;
    float gamepadAxisActionThreshold_ = 0.5f;
    std::unordered_map<SDL_Keycode, std::string> actionKeyNames_;

    // Event bus listeners
    void OnKeyPressed(const events::Event& event);
    void OnKeyReleased(const events::Event& event);
    void OnMouseMoved(const events::Event& event);
    void OnMouseButtonPressed(const events::Event& event);
    void OnMouseButtonReleased(const events::Event& event);
    void OnMouseWheel(const events::Event& event);
    void OnTextInput(const events::Event& event);
    void OnWindowResized(const events::Event& event);
    void OnWindowFocusGained(const events::Event& event);
    void OnWindowFocusLost(const events::Event& event);
    void OnMouseGrabChanged(const events::Event& event);
    void EnsureGamepadSubsystem();
    void TryOpenGamepad();
    void CloseGamepad();
    void UpdateGamepadSnapshot();
    void BuildActionKeyMapping();
    void ApplyKeyMapping(SDL_Keycode key, bool isDown);
    bool IsActionKeyPressed(const std::string& action) const;
    bool ShouldCaptureMouseDelta() const;
    void UpdateMousePosition(float x, float y, float deltaX, float deltaY);
    void ClampGuiCursor();

    // GUI key mapping (extracted from old Sdl3App)
    static const std::unordered_map<SDL_Keycode, std::string> kGuiKeyNames;
};

}  // namespace sdl3cpp::services::impl
