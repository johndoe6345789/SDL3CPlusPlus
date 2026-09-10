#include "services/interfaces/workflow/input/input_button_combine_helpers.hpp"

#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

namespace {

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

bool IsMouseButtonPressed(const std::string& btnStr,
                          const WorkflowContext& context) {
    if (btnStr == "left") return context.Get<bool>("input.mouse.left", false);
    if (btnStr == "right") {
        return context.Get<bool>("input.mouse.right", false);
    }
    if (btnStr == "middle") {
        return context.Get<bool>("input.mouse.middle", false);
    }
    return false;
}

}  // namespace

nlohmann::json LoadButtonAggregationConfig(const WorkflowContext& context,
                                           const std::string& configPath) {
    const auto* contextConfig =
        context.TryGet<nlohmann::json>("input.aggregation.config");
    if (contextConfig && contextConfig->is_object()) {
        return *contextConfig;
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error(
            "input.button.combine: Failed to open config: " + configPath);
    }
    nlohmann::json aggregationConfig;
    configFile >> aggregationConfig;
    return aggregationConfig;
}

bool IsSourcePressed(const nlohmann::json& source,
                     const nlohmann::json* keyState, bool gamepadConnected,
                     const WorkflowContext& context) {
    if (!source.is_object() || !source.contains("type")) return false;
    const std::string sourceType = source["type"].get<std::string>();

    if (sourceType == "key") {
        const std::string keyName = source.value("key", "");
        return keyState && keyState->contains(keyName) &&
               (*keyState)[keyName].get<bool>();
    }
    if (sourceType == "mouse_button") {
        return IsMouseButtonPressed(source.value("button", ""), context);
    }
    if (sourceType == "gamepad_button") {
        return gamepadConnected &&
               IsGamepadButtonPressed(source.value("button", ""), source,
                                      context);
    }
    return false;
}

void WriteButtonOutputs(const nlohmann::json& buttonBinding, bool pressed,
                        WorkflowContext& context) {
    if (!buttonBinding.contains("outputs") ||
        !buttonBinding["outputs"].is_array()) {
        return;
    }
    for (const auto& output : buttonBinding["outputs"]) {
        if (output.is_string()) {
            context.Set<bool>(output.get<std::string>(), pressed);
        }
    }
}

}  // namespace sdl3cpp::services::impl
