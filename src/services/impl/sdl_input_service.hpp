#pragma once

#include "../interfaces/i_input_service.hpp"
#include "../../events/event_bus.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief SDL3-based input service implementation.
 *
 * Subscribes to input events from the event bus and maintains
 * the current input state for queries.
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
    explicit SdlInputService(std::shared_ptr<events::EventBus> eventBus);

    // IInputService interface
    void ProcessEvent(const SDL_Event& event) override;
    void ResetFrameState() override;
    const InputState& GetState() const override { return state_; }
    bool IsKeyPressed(SDL_Keycode key) const override;
    bool IsMouseButtonPressed(uint8_t button) const override;
    std::pair<float, float> GetMousePosition() const override;

private:
    std::shared_ptr<events::EventBus> eventBus_;
    InputState state_;

    // Event bus listeners
    void OnKeyPressed(const events::Event& event);
    void OnKeyReleased(const events::Event& event);
    void OnMouseMoved(const events::Event& event);
    void OnMouseButtonPressed(const events::Event& event);
    void OnMouseButtonReleased(const events::Event& event);
    void OnMouseWheel(const events::Event& event);
    void OnTextInput(const events::Event& event);
};

}  // namespace sdl3cpp::services::impl
