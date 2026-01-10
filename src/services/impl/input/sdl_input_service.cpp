#include "sdl_input_service.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
constexpr float kAxisPositiveMax = static_cast<float>(SDL_JOYSTICK_AXIS_MAX);
constexpr float kAxisNegativeMax = static_cast<float>(-SDL_JOYSTICK_AXIS_MIN);

float NormalizeAxis(Sint16 value) {
    if (value >= 0) {
        float normalized = static_cast<float>(value) / kAxisPositiveMax;
        return normalized > 1.0f ? 1.0f : normalized;
    }
    float normalized = static_cast<float>(value) / kAxisNegativeMax;
    return normalized < -1.0f ? -1.0f : normalized;
}
}  // namespace

namespace sdl3cpp::services::impl {

// GUI key mapping extracted from old Sdl3App
const std::unordered_map<SDL_Keycode, std::string> SdlInputService::kGuiKeyNames = {
    {SDLK_LEFT, "left"}, {SDLK_RIGHT, "right"}, {SDLK_UP, "up"}, {SDLK_DOWN, "down"},
    {SDLK_HOME, "home"}, {SDLK_END, "end"}, {SDLK_BACKSPACE, "backspace"},
    {SDLK_DELETE, "delete"}, {SDLK_RETURN, "return"}, {SDLK_TAB, "tab"},
    {SDLK_ESCAPE, "escape"}, {SDLK_LCTRL, "lctrl"}, {SDLK_RCTRL, "rctrl"},
    {SDLK_LSHIFT, "lshift"}, {SDLK_RSHIFT, "rshift"}, {SDLK_LALT, "lalt"},
    {SDLK_RALT, "ralt"}
};

SdlInputService::SdlInputService(std::shared_ptr<events::IEventBus> eventBus,
                                 std::shared_ptr<IConfigService> configService,
                                 std::shared_ptr<ILogger> logger)
    : eventBus_(std::move(eventBus)),
      configService_(std::move(configService)),
      logger_(logger) {

    // Subscribe to input events
    eventBus_->Subscribe(events::EventType::KeyPressed, [this](const events::Event& e) {
        OnKeyPressed(e);
    });

    eventBus_->Subscribe(events::EventType::KeyReleased, [this](const events::Event& e) {
        OnKeyReleased(e);
    });

    eventBus_->Subscribe(events::EventType::MouseMoved, [this](const events::Event& e) {
        OnMouseMoved(e);
    });

    eventBus_->Subscribe(events::EventType::MouseButtonPressed, [this](const events::Event& e) {
        OnMouseButtonPressed(e);
    });

    eventBus_->Subscribe(events::EventType::MouseButtonReleased, [this](const events::Event& e) {
        OnMouseButtonReleased(e);
    });

    eventBus_->Subscribe(events::EventType::MouseWheel, [this](const events::Event& e) {
        OnMouseWheel(e);
    });

    eventBus_->Subscribe(events::EventType::TextInput, [this](const events::Event& e) {
        OnTextInput(e);
    });

    eventBus_->Subscribe(events::EventType::WindowResized, [this](const events::Event& e) {
        OnWindowResized(e);
    });

    eventBus_->Subscribe(events::EventType::WindowFocusGained, [this](const events::Event& e) {
        OnWindowFocusGained(e);
    });

    eventBus_->Subscribe(events::EventType::WindowFocusLost, [this](const events::Event& e) {
        OnWindowFocusLost(e);
    });

    eventBus_->Subscribe(events::EventType::MouseGrabChanged, [this](const events::Event& e) {
        OnMouseGrabChanged(e);
    });

    if (logger_) {
        logger_->Trace("SdlInputService", "SdlInputService",
                       "eventBus=" + std::string(eventBus_ ? "set" : "null"));
    }
    if (configService_) {
        const auto& mouseGrabConfig = configService_->GetMouseGrabConfig();
        mouseGrabGatesLook_ = mouseGrabConfig.enabled &&
                              (mouseGrabConfig.grabOnClick || mouseGrabConfig.startGrabbed);
        mouseRelativeMode_ = mouseGrabConfig.relativeMode;
        guiWindowWidth_ = configService_->GetWindowWidth();
        guiWindowHeight_ = configService_->GetWindowHeight();
        if (guiWindowWidth_ > 0 && guiWindowHeight_ > 0) {
            guiCursorX_ = static_cast<float>(guiWindowWidth_) * 0.5f;
            guiCursorY_ = static_cast<float>(guiWindowHeight_) * 0.5f;
            state_.mouseX = guiCursorX_;
            state_.mouseY = guiCursorY_;
            guiInputSnapshot_.mouseX = guiCursorX_;
            guiInputSnapshot_.mouseY = guiCursorY_;
        }
        if (logger_) {
            logger_->Trace("SdlInputService", "SdlInputService",
                           "mouseGrabGatesLook=" + std::string(mouseGrabGatesLook_ ? "true" : "false") +
                           ", relativeMode=" + std::string(mouseRelativeMode_ ? "true" : "false") +
                           ", guiWindow=" + std::to_string(guiWindowWidth_) + "x" +
                           std::to_string(guiWindowHeight_));
        }
    }
    BuildActionKeyMapping();
    EnsureGamepadSubsystem();
}

SdlInputService::~SdlInputService() {
    CloseGamepad();
}

void SdlInputService::ProcessEvent(const SDL_Event& event) {
    if (logger_) {
        logger_->Trace("SdlInputService", "ProcessEvent",
                       "eventType=" + std::to_string(static_cast<int>(event.type)));
    }
    // This method allows direct event processing if needed
    // (though typically events flow through the event bus)
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            state_.keysPressed.insert(event.key.key);
            ApplyKeyMapping(event.key.key, true);
            break;

        case SDL_EVENT_KEY_UP:
            state_.keysPressed.erase(event.key.key);
            ApplyKeyMapping(event.key.key, false);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            UpdateMousePosition(static_cast<float>(event.motion.x),
                                static_cast<float>(event.motion.y),
                                static_cast<float>(event.motion.xrel),
                                static_cast<float>(event.motion.yrel));
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            state_.mouseButtonsPressed.insert(event.button.button);
            // GUI input processing
            if (event.button.button == SDL_BUTTON_LEFT) {
                guiInputSnapshot_.mouseDown = true;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            state_.mouseButtonsPressed.erase(event.button.button);
            // GUI input processing
            if (event.button.button == SDL_BUTTON_LEFT) {
                guiInputSnapshot_.mouseDown = false;
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            state_.mouseWheelDeltaX = event.wheel.x;
            state_.mouseWheelDeltaY = event.wheel.y;
            // GUI input processing
            guiInputSnapshot_.wheel += static_cast<float>(event.wheel.y);
            break;

        case SDL_EVENT_TEXT_INPUT:
            state_.textInput += event.text.text;
            // GUI input processing
            guiInputSnapshot_.textInput.append(event.text.text);
            break;

        default:
            break;
    }
}

void SdlInputService::ResetFrameState() {
    if (logger_) {
        logger_->Trace("SdlInputService", "ResetFrameState");
    }
    // Reset per-frame state
    state_.mouseDeltaX = 0.0f;
    state_.mouseDeltaY = 0.0f;
    state_.mouseWheelDeltaX = 0.0f;
    state_.mouseWheelDeltaY = 0.0f;
    state_.textInput.clear();

    // Reset GUI per-frame state
    guiInputSnapshot_.mouseDeltaX = 0.0f;
    guiInputSnapshot_.mouseDeltaY = 0.0f;
    guiInputSnapshot_.wheel = 0.0f;
    guiInputSnapshot_.textInput.clear();
}

bool SdlInputService::IsKeyPressed(SDL_Keycode key) const {
    if (logger_) {
        logger_->Trace("SdlInputService", "IsKeyPressed",
                       "key=" + std::to_string(static_cast<int>(key)));
    }
    return state_.keysPressed.count(key) > 0;
}

bool SdlInputService::IsMouseButtonPressed(uint8_t button) const {
    if (logger_) {
        logger_->Trace("SdlInputService", "IsMouseButtonPressed",
                       "button=" + std::to_string(static_cast<int>(button)));
    }
    return state_.mouseButtonsPressed.count(button) > 0;
}

bool SdlInputService::IsActionPressed(const std::string& action) const {
    if (logger_) {
        logger_->Trace("SdlInputService", "IsActionPressed",
                       "action=" + action);
    }
    return IsActionKeyPressed(action);
}

std::pair<float, float> SdlInputService::GetMousePosition() const {
    if (logger_) {
        logger_->Trace("SdlInputService", "GetMousePosition");
    }
    return {state_.mouseX, state_.mouseY};
}

void SdlInputService::SetRelativeMouseMode(bool enabled) {
    if (logger_) {
        logger_->Trace("SdlInputService", "SetRelativeMouseMode",
                       "enabled=" + std::string(enabled ? "true" : "false"));
    }
    mouseRelativeMode_ = enabled;
}

bool SdlInputService::IsRelativeMouseMode() const {
    if (logger_) {
        logger_->Trace("SdlInputService", "IsRelativeMouseMode");
    }
    return mouseRelativeMode_;
}

void SdlInputService::OnKeyPressed(const events::Event& event) {
    const auto& keyEvent = event.GetData<events::KeyEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnKeyPressed",
                       "key=" + std::to_string(static_cast<int>(keyEvent.key)) +
                       ", scancode=" + std::to_string(static_cast<int>(keyEvent.scancode)) +
                       ", modifiers=" + std::to_string(static_cast<int>(keyEvent.modifiers)) +
                       ", repeat=" + std::string(keyEvent.repeat ? "true" : "false"));
    }
    state_.keysPressed.insert(keyEvent.key);
    ApplyKeyMapping(keyEvent.key, true);
}

void SdlInputService::OnKeyReleased(const events::Event& event) {
    const auto& keyEvent = event.GetData<events::KeyEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnKeyReleased",
                       "key=" + std::to_string(static_cast<int>(keyEvent.key)) +
                       ", scancode=" + std::to_string(static_cast<int>(keyEvent.scancode)) +
                       ", modifiers=" + std::to_string(static_cast<int>(keyEvent.modifiers)) +
                       ", repeat=" + std::string(keyEvent.repeat ? "true" : "false"));
    }
    state_.keysPressed.erase(keyEvent.key);
    ApplyKeyMapping(keyEvent.key, false);
}

void SdlInputService::OnMouseMoved(const events::Event& event) {
    const auto& mouseEvent = event.GetData<events::MouseMovedEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnMouseMoved",
                       "x=" + std::to_string(mouseEvent.x) +
                       ", y=" + std::to_string(mouseEvent.y) +
                       ", deltaX=" + std::to_string(mouseEvent.deltaX) +
                       ", deltaY=" + std::to_string(mouseEvent.deltaY));
    }
    UpdateMousePosition(mouseEvent.x, mouseEvent.y, mouseEvent.deltaX, mouseEvent.deltaY);
}

void SdlInputService::OnMouseButtonPressed(const events::Event& event) {
    const auto& buttonEvent = event.GetData<events::MouseButtonEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnMouseButtonPressed",
                       "button=" + std::to_string(static_cast<int>(buttonEvent.button)) +
                       ", clicks=" + std::to_string(static_cast<int>(buttonEvent.clicks)) +
                       ", x=" + std::to_string(buttonEvent.x) +
                       ", y=" + std::to_string(buttonEvent.y));
    }
    state_.mouseButtonsPressed.insert(buttonEvent.button);
    if (buttonEvent.button == SDL_BUTTON_LEFT) {
        guiInputSnapshot_.mouseDown = true;
    }
}

void SdlInputService::OnMouseButtonReleased(const events::Event& event) {
    const auto& buttonEvent = event.GetData<events::MouseButtonEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnMouseButtonReleased",
                       "button=" + std::to_string(static_cast<int>(buttonEvent.button)) +
                       ", clicks=" + std::to_string(static_cast<int>(buttonEvent.clicks)) +
                       ", x=" + std::to_string(buttonEvent.x) +
                       ", y=" + std::to_string(buttonEvent.y));
    }
    state_.mouseButtonsPressed.erase(buttonEvent.button);
    if (buttonEvent.button == SDL_BUTTON_LEFT) {
        guiInputSnapshot_.mouseDown = false;
    }
}

void SdlInputService::OnMouseWheel(const events::Event& event) {
    const auto& wheelEvent = event.GetData<events::MouseWheelEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnMouseWheel",
                       "deltaX=" + std::to_string(wheelEvent.deltaX) +
                       ", deltaY=" + std::to_string(wheelEvent.deltaY) +
                       ", flipped=" + std::string(wheelEvent.flipped ? "true" : "false"));
    }
    state_.mouseWheelDeltaX += wheelEvent.deltaX;
    state_.mouseWheelDeltaY += wheelEvent.deltaY;
    guiInputSnapshot_.wheel += wheelEvent.deltaY;
}

void SdlInputService::OnTextInput(const events::Event& event) {
    const auto& textEvent = event.GetData<events::TextInputEvent>();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnTextInput",
                       "text=" + textEvent.text);
    }
    state_.textInput += textEvent.text;
    guiInputSnapshot_.textInput += textEvent.text;
}

void SdlInputService::OnWindowResized(const events::Event& event) {
    const auto& resized = event.GetData<events::WindowResizedEvent>();
    guiWindowWidth_ = resized.width;
    guiWindowHeight_ = resized.height;
    ClampGuiCursor();
    if (logger_) {
        logger_->Trace("SdlInputService", "OnWindowResized",
                       "width=" + std::to_string(guiWindowWidth_) +
                       ", height=" + std::to_string(guiWindowHeight_));
    }
}

void SdlInputService::OnWindowFocusGained(const events::Event& event) {
    (void)event;
    windowFocused_ = true;
    if (logger_) {
        logger_->Trace("SdlInputService", "OnWindowFocusGained", "windowFocused=true");
    }
}

void SdlInputService::OnWindowFocusLost(const events::Event& event) {
    (void)event;
    windowFocused_ = false;
    guiInputSnapshot_.mouseDeltaX = 0.0f;
    guiInputSnapshot_.mouseDeltaY = 0.0f;
    if (logger_) {
        logger_->Trace("SdlInputService", "OnWindowFocusLost", "windowFocused=false");
    }
}

void SdlInputService::OnMouseGrabChanged(const events::Event& event) {
    const auto& grabEvent = event.GetData<events::MouseGrabEvent>();
    mouseGrabbed_ = grabEvent.grabbed;
    if (!mouseGrabbed_) {
        guiInputSnapshot_.mouseDeltaX = 0.0f;
        guiInputSnapshot_.mouseDeltaY = 0.0f;
    }
    if (logger_) {
        logger_->Trace("SdlInputService", "OnMouseGrabChanged",
                       "grabbed=" + std::string(mouseGrabbed_ ? "true" : "false"));
    }
}

void SdlInputService::BuildActionKeyMapping() {
    actionKeyNames_.clear();
    gamepadButtonActions_.clear();
    gamepadAxisActions_.clear();
    if (!configService_) {
        if (logger_) {
            logger_->Trace("SdlInputService", "BuildActionKeyMapping", "configService=null");
        }
        return;
    }

    const auto& bindings = configService_->GetInputBindings();
    auto addKey = [&](const char* actionName, const std::string& keyName) {
        if (keyName.empty()) {
            return;
        }
        SDL_Keycode key = SDL_GetKeyFromName(keyName.c_str());
        if (key == SDLK_UNKNOWN) {
            if (logger_) {
                logger_->Error("SdlInputService: unknown key binding for " + std::string(actionName) +
                               " -> " + keyName);
            }
            return;
        }
        actionKeyNames_[key] = actionName;
        if (logger_) {
            logger_->Trace("SdlInputService", "BuildActionKeyMapping",
                           "action=" + std::string(actionName) +
                           ", keyName=" + keyName +
                           ", keyCode=" + std::to_string(static_cast<int>(key)));
        }
    };

    addKey("move_forward", bindings.moveForwardKey);
    addKey("move_back", bindings.moveBackKey);
    addKey("move_left", bindings.moveLeftKey);
    addKey("move_right", bindings.moveRightKey);
    addKey("fly_up", bindings.flyUpKey);
    addKey("fly_down", bindings.flyDownKey);
    addKey("jump", bindings.jumpKey);
    addKey("noclip_toggle", bindings.noclipToggleKey);
    addKey("music_toggle", bindings.musicToggleKey);

    auto toLower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    };

    auto setButton = [&](const char* bindingName, const std::string& buttonValue, SDL_GamepadButton& target) {
        if (buttonValue.empty()) {
            return;
        }
        std::string normalized = toLower(buttonValue);
        SDL_GamepadButton button = SDL_GetGamepadButtonFromString(normalized.c_str());
        if (button != SDL_GAMEPAD_BUTTON_INVALID) {
            target = button;
            if (logger_) {
                logger_->Trace("SdlInputService", "BuildActionKeyMapping",
                               "gamepadButton=" + std::string(bindingName) +
                               ", value=" + buttonValue +
                               ", buttonCode=" + std::to_string(static_cast<int>(button)));
            }
        } else if (logger_) {
            logger_->Error("SdlInputService: unknown gamepad button binding for " +
                           std::string(bindingName) + " -> " + buttonValue);
        }
    };

    setButton("music_toggle_gamepad", bindings.musicToggleGamepadButton, musicToggleButton_);
    setButton("gamepad_dpad_up", bindings.gamepadDpadUpButton, dpadUpButton_);
    setButton("gamepad_dpad_down", bindings.gamepadDpadDownButton, dpadDownButton_);
    setButton("gamepad_dpad_left", bindings.gamepadDpadLeftButton, dpadLeftButton_);
    setButton("gamepad_dpad_right", bindings.gamepadDpadRightButton, dpadRightButton_);

    auto setAxis = [&](const char* axisName, const std::string& axisValue, SDL_GamepadAxis& target) {
        if (axisValue.empty()) {
            return;
        }
        std::string normalized = toLower(axisValue);
        SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(normalized.c_str());
        if (axis != SDL_GAMEPAD_AXIS_INVALID) {
            target = axis;
            if (logger_) {
                logger_->Trace("SdlInputService", "BuildActionKeyMapping",
                               "gamepadAxis=" + std::string(axisName) +
                               ", value=" + axisValue +
                               ", axisCode=" + std::to_string(static_cast<int>(axis)));
            }
        } else if (logger_) {
            logger_->Error("SdlInputService: unknown gamepad axis binding for " +
                           std::string(axisName) + " -> " + axisValue);
        }
    };

    setAxis("gamepad_move_x_axis", bindings.gamepadMoveXAxis, moveXAxis_);
    setAxis("gamepad_move_y_axis", bindings.gamepadMoveYAxis, moveYAxis_);
    setAxis("gamepad_look_x_axis", bindings.gamepadLookXAxis, lookXAxis_);
    setAxis("gamepad_look_y_axis", bindings.gamepadLookYAxis, lookYAxis_);

    for (const auto& [buttonName, actionName] : bindings.gamepadButtonActions) {
        if (buttonName.empty() || actionName.empty()) {
            continue;
        }
        std::string normalized = toLower(buttonName);
        SDL_GamepadButton button = SDL_GetGamepadButtonFromString(normalized.c_str());
        if (button == SDL_GAMEPAD_BUTTON_INVALID) {
            if (logger_) {
                logger_->Error("SdlInputService: unknown gamepad button mapping " +
                               buttonName + " -> " + actionName);
            }
            continue;
        }
        gamepadButtonActions_[button] = actionName;
        if (logger_) {
            logger_->Trace("SdlInputService", "BuildActionKeyMapping",
                           "gamepadButtonAction=" + buttonName +
                           ", action=" + actionName +
                           ", buttonCode=" + std::to_string(static_cast<int>(button)));
        }
    }

    for (const auto& [axisName, actionName] : bindings.gamepadAxisActions) {
        if (axisName.empty() || actionName.empty()) {
            continue;
        }
        std::string normalized = toLower(axisName);
        SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(normalized.c_str());
        if (axis == SDL_GAMEPAD_AXIS_INVALID) {
            if (logger_) {
                logger_->Error("SdlInputService: unknown gamepad axis mapping " +
                               axisName + " -> " + actionName);
            }
            continue;
        }
        gamepadAxisActions_[axis] = actionName;
        if (logger_) {
            logger_->Trace("SdlInputService", "BuildActionKeyMapping",
                           "gamepadAxisAction=" + axisName +
                           ", action=" + actionName +
                           ", axisCode=" + std::to_string(static_cast<int>(axis)));
        }
    }

    gamepadAxisActionThreshold_ = bindings.gamepadAxisActionThreshold;
    if (gamepadAxisActionThreshold_ < 0.0f) {
        gamepadAxisActionThreshold_ = 0.0f;
    } else if (gamepadAxisActionThreshold_ > 1.0f) {
        gamepadAxisActionThreshold_ = 1.0f;
    }

    if (logger_) {
        logger_->Trace("SdlInputService", "BuildActionKeyMapping",
                       "musicToggleButton=" + std::to_string(static_cast<int>(musicToggleButton_)) +
                       ", dpadUpButton=" + std::to_string(static_cast<int>(dpadUpButton_)) +
                       ", dpadDownButton=" + std::to_string(static_cast<int>(dpadDownButton_)) +
                       ", dpadLeftButton=" + std::to_string(static_cast<int>(dpadLeftButton_)) +
                       ", dpadRightButton=" + std::to_string(static_cast<int>(dpadRightButton_)) +
                       ", moveXAxis=" + std::to_string(static_cast<int>(moveXAxis_)) +
                       ", moveYAxis=" + std::to_string(static_cast<int>(moveYAxis_)) +
                       ", lookXAxis=" + std::to_string(static_cast<int>(lookXAxis_)) +
                       ", lookYAxis=" + std::to_string(static_cast<int>(lookYAxis_)) +
                       ", buttonActions=" + std::to_string(gamepadButtonActions_.size()) +
                       ", axisActions=" + std::to_string(gamepadAxisActions_.size()) +
                       ", axisThreshold=" + std::to_string(gamepadAxisActionThreshold_));
    }
}

void SdlInputService::ApplyKeyMapping(SDL_Keycode key, bool isDown) {
    auto actionIt = actionKeyNames_.find(key);
    if (actionIt != actionKeyNames_.end()) {
        guiInputSnapshot_.keyStates[actionIt->second] = isDown;
    }
    auto guiIt = kGuiKeyNames.find(key);
    if (guiIt != kGuiKeyNames.end()) {
        guiInputSnapshot_.keyStates[guiIt->second] = isDown;
    }
}

bool SdlInputService::IsActionKeyPressed(const std::string& action) const {
    for (const auto& key : state_.keysPressed) {
        auto it = actionKeyNames_.find(key);
        if (it != actionKeyNames_.end() && it->second == action) {
            return true;
        }
    }
    return false;
}

bool SdlInputService::ShouldCaptureMouseDelta() const {
    if (!windowFocused_) {
        return false;
    }
    if (mouseGrabGatesLook_) {
        return mouseGrabbed_;
    }
    return true;
}

void SdlInputService::UpdateMousePosition(float x, float y, float deltaX, float deltaY) {
    if (mouseRelativeMode_ && mouseGrabbed_) {
        guiCursorX_ += deltaX;
        guiCursorY_ += deltaY;
        ClampGuiCursor();
        state_.mouseX = guiCursorX_;
        state_.mouseY = guiCursorY_;
        guiInputSnapshot_.mouseX = guiCursorX_;
        guiInputSnapshot_.mouseY = guiCursorY_;
        if (logger_ && !relativeCursorLogged_) {
            logger_->Trace("SdlInputService", "UpdateMousePosition",
                           "relativeCursor=true, startX=" + std::to_string(guiCursorX_) +
                           ", startY=" + std::to_string(guiCursorY_));
            relativeCursorLogged_ = true;
        }
    } else {
        state_.mouseX = x;
        state_.mouseY = y;
        guiInputSnapshot_.mouseX = x;
        guiInputSnapshot_.mouseY = y;
        guiCursorX_ = x;
        guiCursorY_ = y;
    }

    if (ShouldCaptureMouseDelta()) {
        guiInputSnapshot_.mouseDeltaX += deltaX;
        guiInputSnapshot_.mouseDeltaY += deltaY;
    }
    state_.mouseDeltaX += deltaX;
    state_.mouseDeltaY += deltaY;
}

void SdlInputService::ClampGuiCursor() {
    if (guiWindowWidth_ == 0 || guiWindowHeight_ == 0) {
        return;
    }
    guiCursorX_ = std::clamp(guiCursorX_, 0.0f, static_cast<float>(guiWindowWidth_ - 1));
    guiCursorY_ = std::clamp(guiCursorY_, 0.0f, static_cast<float>(guiWindowHeight_ - 1));
}

void SdlInputService::EnsureGamepadSubsystem() {
    uint32_t initialized = SDL_WasInit(0);
    if ((initialized & SDL_INIT_GAMEPAD) != 0) {
        return;
    }
    bool result = false;
    if (initialized == 0) {
        result = SDL_Init(SDL_INIT_GAMEPAD);
    } else {
        result = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    }
    if (!result && logger_) {
        logger_->Error("SdlInputService: SDL_INIT_GAMEPAD failed: " + std::string(SDL_GetError()));
    }
}

void SdlInputService::TryOpenGamepad() {
    if (gamepad_) {
        return;
    }
    EnsureGamepadSubsystem();
    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
    if (!gamepads || count <= 0) {
        if (gamepads) {
            SDL_free(gamepads);
        }
        return;
    }
    for (int i = 0; i < count; ++i) {
        if (!SDL_IsGamepad(gamepads[i])) {
            continue;
        }
        gamepad_ = SDL_OpenGamepad(gamepads[i]);
        if (gamepad_) {
            break;
        }
    }
    SDL_free(gamepads);
}

void SdlInputService::CloseGamepad() {
    if (gamepad_) {
        SDL_CloseGamepad(gamepad_);
        gamepad_ = nullptr;
    }
}

void SdlInputService::UpdateGamepadSnapshot() {
    if (!gamepad_) {
        TryOpenGamepad();
    }

    if (!gamepad_) {
        guiInputSnapshot_.gamepadConnected = false;
        guiInputSnapshot_.gamepadLeftX = 0.0f;
        guiInputSnapshot_.gamepadLeftY = 0.0f;
        guiInputSnapshot_.gamepadRightX = 0.0f;
        guiInputSnapshot_.gamepadRightY = 0.0f;
        guiInputSnapshot_.gamepadTogglePressed = false;
        return;
    }

    SDL_JoystickConnectionState connection = SDL_GetGamepadConnectionState(gamepad_);
    if (connection == SDL_JOYSTICK_CONNECTION_INVALID) {
        CloseGamepad();
        guiInputSnapshot_.gamepadConnected = false;
        guiInputSnapshot_.gamepadLeftX = 0.0f;
        guiInputSnapshot_.gamepadLeftY = 0.0f;
        guiInputSnapshot_.gamepadRightX = 0.0f;
        guiInputSnapshot_.gamepadRightY = 0.0f;
        guiInputSnapshot_.gamepadTogglePressed = false;
        return;
    }

    guiInputSnapshot_.gamepadConnected = true;
    guiInputSnapshot_.gamepadLeftX = 0.0f;
    guiInputSnapshot_.gamepadLeftY = 0.0f;
    guiInputSnapshot_.gamepadRightX = 0.0f;
    guiInputSnapshot_.gamepadRightY = 0.0f;
    if (moveXAxis_ != SDL_GAMEPAD_AXIS_INVALID) {
        guiInputSnapshot_.gamepadLeftX = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, moveXAxis_));
    }
    if (moveYAxis_ != SDL_GAMEPAD_AXIS_INVALID) {
        guiInputSnapshot_.gamepadLeftY = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, moveYAxis_));
    }
    if (lookXAxis_ != SDL_GAMEPAD_AXIS_INVALID) {
        guiInputSnapshot_.gamepadRightX = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, lookXAxis_));
    }
    if (lookYAxis_ != SDL_GAMEPAD_AXIS_INVALID) {
        guiInputSnapshot_.gamepadRightY = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, lookYAxis_));
    }
    guiInputSnapshot_.gamepadTogglePressed =
        SDL_GetGamepadButton(gamepad_, musicToggleButton_);

    auto updateActionState = [&](const std::string& actionName, bool gamepadPressed) {
        bool keyboardPressed = IsActionKeyPressed(actionName);
        bool current = guiInputSnapshot_.keyStates[actionName];
        guiInputSnapshot_.keyStates[actionName] = current || keyboardPressed || gamepadPressed;
    };

    updateActionState("move_forward", SDL_GetGamepadButton(gamepad_, dpadUpButton_));
    updateActionState("move_back", SDL_GetGamepadButton(gamepad_, dpadDownButton_));
    updateActionState("move_left", SDL_GetGamepadButton(gamepad_, dpadLeftButton_));
    updateActionState("move_right", SDL_GetGamepadButton(gamepad_, dpadRightButton_));

    for (const auto& [button, actionName] : gamepadButtonActions_) {
        updateActionState(actionName, SDL_GetGamepadButton(gamepad_, button));
    }

    for (const auto& [axis, actionName] : gamepadAxisActions_) {
        float value = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, axis));
        bool pressed = std::fabs(value) >= gamepadAxisActionThreshold_;
        updateActionState(actionName, pressed);
    }
}

void SdlInputService::UpdateGuiInput() {
    if (logger_) {
        logger_->Trace("SdlInputService", "UpdateGuiInput");
    }
    UpdateGamepadSnapshot();
}

}  // namespace sdl3cpp::services::impl
