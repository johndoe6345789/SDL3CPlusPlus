#include "sdl_input_service.hpp"

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
    {SDLK_RALT, "ralt"}, {SDLK_w, "w"}, {SDLK_a, "a"}, {SDLK_s, "s"},
    {SDLK_d, "d"}, {SDLK_m, "m"}
};

SdlInputService::SdlInputService(std::shared_ptr<events::IEventBus> eventBus, std::shared_ptr<ILogger> logger)
    : eventBus_(std::move(eventBus)), logger_(logger) {

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

    if (logger_) {
        logger_->Trace("SdlInputService", "SdlInputService",
                       "eventBus=" + std::string(eventBus_ ? "set" : "null"));
    }
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
            // GUI input processing
            {
                auto it = kGuiKeyNames.find(event.key.key);
                if (it != kGuiKeyNames.end()) {
                    guiInputSnapshot_.keyStates[it->second] = true;
                }
            }
            break;

        case SDL_EVENT_KEY_UP:
            state_.keysPressed.erase(event.key.key);
            // GUI input processing
            {
                auto it = kGuiKeyNames.find(event.key.key);
                if (it != kGuiKeyNames.end()) {
                    guiInputSnapshot_.keyStates[it->second] = false;
                }
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            state_.mouseX = event.motion.x;
            state_.mouseY = event.motion.y;
            // GUI input processing
            guiInputSnapshot_.mouseX = static_cast<float>(event.motion.x);
            guiInputSnapshot_.mouseY = static_cast<float>(event.motion.y);
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
    state_.mouseWheelDeltaX = 0.0f;
    state_.mouseWheelDeltaY = 0.0f;
    state_.textInput.clear();

    // Reset GUI per-frame state
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

std::pair<float, float> SdlInputService::GetMousePosition() const {
    if (logger_) {
        logger_->Trace("SdlInputService", "GetMousePosition");
    }
    return {state_.mouseX, state_.mouseY};
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
    auto it = kGuiKeyNames.find(keyEvent.key);
    if (it != kGuiKeyNames.end()) {
        guiInputSnapshot_.keyStates[it->second] = true;
    }
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
    auto it = kGuiKeyNames.find(keyEvent.key);
    if (it != kGuiKeyNames.end()) {
        guiInputSnapshot_.keyStates[it->second] = false;
    }
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
    state_.mouseX = mouseEvent.x;
    state_.mouseY = mouseEvent.y;
    guiInputSnapshot_.mouseX = mouseEvent.x;
    guiInputSnapshot_.mouseY = mouseEvent.y;
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
    guiInputSnapshot_.gamepadLeftX = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX));
    guiInputSnapshot_.gamepadLeftY = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY));
    guiInputSnapshot_.gamepadRightX = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX));
    guiInputSnapshot_.gamepadRightY = NormalizeAxis(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY));
    guiInputSnapshot_.gamepadTogglePressed =
        SDL_GetGamepadButton(gamepad_, SDL_GAMEPAD_BUTTON_START);
}

void SdlInputService::SetGuiScriptService(IGuiScriptService* guiScriptService) {
    if (logger_) {
        logger_->Trace("SdlInputService", "SetGuiScriptService",
                       "guiScriptServiceIsNull=" + std::string(guiScriptService ? "false" : "true"));
    }
    guiScriptService_ = guiScriptService;
}

void SdlInputService::UpdateGuiInput() {
    if (logger_) {
        logger_->Trace("SdlInputService", "UpdateGuiInput",
                       "guiScriptServiceIsNull=" + std::string(guiScriptService_ ? "false" : "true"));
    }
    if (guiScriptService_) {
        UpdateGamepadSnapshot();
        guiScriptService_->UpdateGuiInput(guiInputSnapshot_);
    }
}

}  // namespace sdl3cpp::services::impl
