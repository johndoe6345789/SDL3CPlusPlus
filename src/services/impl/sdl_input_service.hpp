#pragma once

#include "../interfaces/i_input_service.hpp"
#include "../interfaces/i_gui_script_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../events/i_event_bus.hpp"
#include <memory>

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
    explicit SdlInputService(std::shared_ptr<events::IEventBus> eventBus, std::shared_ptr<ILogger> logger);

    // IInputService interface
    void ProcessEvent(const SDL_Event& event) override;
    void ResetFrameState() override;
    const InputState& GetState() const override { return state_; }
    bool IsKeyPressed(SDL_Keycode key) const override;
    bool IsMouseButtonPressed(uint8_t button) const override;
    std::pair<float, float> GetMousePosition() const override;
    void SetGuiScriptService(IGuiScriptService* guiScriptService) override;
    void UpdateGuiInput() override;

private:
    std::shared_ptr<events::IEventBus> eventBus_;
    std::shared_ptr<ILogger> logger_;
    InputState state_;
    GuiInputSnapshot guiInputSnapshot_;
    IGuiScriptService* guiScriptService_ = nullptr;

    // Event bus listeners
    void OnKeyPressed(const events::Event& event);
    void OnKeyReleased(const events::Event& event);
    void OnMouseMoved(const events::Event& event);
    void OnMouseButtonPressed(const events::Event& event);
    void OnMouseButtonReleased(const events::Event& event);
    void OnMouseWheel(const events::Event& event);
    void OnTextInput(const events::Event& event);

    // GUI key mapping (extracted from old Sdl3App)
    static const std::unordered_map<SDL_Keycode, std::string> kGuiKeyNames;
};

}  // namespace sdl3cpp::services::impl
