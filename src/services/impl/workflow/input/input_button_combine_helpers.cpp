#include "services/interfaces/workflow/input/input_button_combine_helpers.hpp"

namespace sdl3cpp::services::impl {

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
