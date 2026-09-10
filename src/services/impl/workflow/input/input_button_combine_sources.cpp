#include "services/interfaces/workflow/input/input_button_combine_sources.hpp"
#include "services/interfaces/workflow/input/input_button_gamepad_source.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

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

}  // namespace sdl3cpp::services::impl
