#include "services/interfaces/workflow/input/input_axis_source_value.hpp"

namespace sdl3cpp::services::impl {

float ReadAxisSourceValue(const nlohmann::json& source,
                          const WorkflowContext& context,
                          const nlohmann::json* keyState,
                          bool gamepadConnected) {
    const std::string sourceType = source["type"].get<std::string>();
    float value                  = 0.0f;

    if (sourceType == "key") {
        std::string keyName = source.value("key", "");
        if (keyState && keyState->contains(keyName) &&
            (*keyState)[keyName].get<bool>()) {
            value = 1.0f;
        }
    } else if (sourceType == "mouse") {
        std::string axis = source.value("axis", "");
        if (axis == "x") {
            value = context.Get<float>("input.mouse.x", 0.0f);
        } else if (axis == "y") {
            value = context.Get<float>("input.mouse.y", 0.0f);
        }
    } else if (sourceType == "gamepad_axis") {
        if (gamepadConnected) {
            std::string axisStr    = source.value("axis", "");
            std::string contextKey = "input.gamepad." + axisStr;
            value                  = context.Get<float>(contextKey, 0.0f);
        }
    }
    return value;
}

}  // namespace sdl3cpp::services::impl
