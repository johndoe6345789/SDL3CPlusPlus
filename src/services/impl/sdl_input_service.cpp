#include "sdl_input_service.hpp"

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
}

void SdlInputService::ProcessEvent(const SDL_Event& event) {
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
    // Reset per-frame state
    state_.mouseWheelDeltaX = 0.0f;
    state_.mouseWheelDeltaY = 0.0f;
    state_.textInput.clear();

    // Reset GUI per-frame state
    guiInputSnapshot_.wheel = 0.0f;
    guiInputSnapshot_.textInput.clear();
}

bool SdlInputService::IsKeyPressed(SDL_Keycode key) const {
    return state_.keysPressed.count(key) > 0;
}

bool SdlInputService::IsMouseButtonPressed(uint8_t button) const {
    return state_.mouseButtonsPressed.count(button) > 0;
}

std::pair<float, float> SdlInputService::GetMousePosition() const {
    return {state_.mouseX, state_.mouseY};
}

void SdlInputService::OnKeyPressed(const events::Event& event) {
    const auto& keyEvent = event.GetData<events::KeyEvent>();
    state_.keysPressed.insert(keyEvent.key);
}

void SdlInputService::OnKeyReleased(const events::Event& event) {
    const auto& keyEvent = event.GetData<events::KeyEvent>();
    state_.keysPressed.erase(keyEvent.key);
}

void SdlInputService::OnMouseMoved(const events::Event& event) {
    const auto& mouseEvent = event.GetData<events::MouseMovedEvent>();
    state_.mouseX = mouseEvent.x;
    state_.mouseY = mouseEvent.y;
}

void SdlInputService::OnMouseButtonPressed(const events::Event& event) {
    const auto& buttonEvent = event.GetData<events::MouseButtonEvent>();
    state_.mouseButtonsPressed.insert(buttonEvent.button);
}

void SdlInputService::OnMouseButtonReleased(const events::Event& event) {
    const auto& buttonEvent = event.GetData<events::MouseButtonEvent>();
    state_.mouseButtonsPressed.erase(buttonEvent.button);
}

void SdlInputService::OnMouseWheel(const events::Event& event) {
    const auto& wheelEvent = event.GetData<events::MouseWheelEvent>();
    state_.mouseWheelDeltaX += wheelEvent.deltaX;
    state_.mouseWheelDeltaY += wheelEvent.deltaY;
}

void SdlInputService::OnTextInput(const events::Event& event) {
    const auto& textEvent = event.GetData<events::TextInputEvent>();
    state_.textInput += textEvent.text;
}

void SdlInputService::SetGuiScriptService(IGuiScriptService* guiScriptService) {
    guiScriptService_ = guiScriptService;
}

void SdlInputService::UpdateGuiInput() {
    if (guiScriptService_) {
        guiScriptService_->UpdateGuiInput(guiInputSnapshot_);
    }
}

}  // namespace sdl3cpp::services::impl
