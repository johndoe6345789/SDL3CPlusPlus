#include "services/interfaces/workflow/input/input_button_gamepad_source.hpp"

namespace sdl3cpp::services::impl {

bool IsGamepadButtonPressed(const std::string& btnStr,
                            const nlohmann::json& source,
                            const WorkflowContext& context) {
    // Map config button names to context keys.
    if (btnStr == "a") {
        return context.Get<bool>("input.gamepad.button_south", false);
    }
    if (btnStr == "b") {
        return context.Get<bool>("input.gamepad.button_east", false);
    }
    if (btnStr == "x") {
        return context.Get<bool>("input.gamepad.button_west", false);
    }
    if (btnStr == "y") {
        return context.Get<bool>("input.gamepad.button_north", false);
    }
    if (btnStr == "lb") {
        return context.Get<bool>("input.gamepad.button_left_shoulder", false);
    }
    if (btnStr == "rb") {
        return context.Get<bool>("input.gamepad.button_right_shoulder", false);
    }
    if (btnStr == "back") {
        return context.Get<bool>("input.gamepad.button_back", false);
    }
    if (btnStr == "start") {
        return context.Get<bool>("input.gamepad.button_start", false);
    }
    if (btnStr == "trigger_left") {
        const float threshold = source.value("threshold", 0.5f);
        return context.Get<float>("input.gamepad.trigger_left", 0.0f) >=
               threshold;
    }
    if (btnStr == "trigger_right") {
        const float threshold = source.value("threshold", 0.5f);
        return context.Get<float>("input.gamepad.trigger_right", 0.0f) >=
               threshold;
    }
    return false;
}

}  // namespace sdl3cpp::services::impl
