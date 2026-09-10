#include "services/interfaces/workflow/input/input_axis_combine_write.hpp"
#include "services/interfaces/workflow/input/input_axis_deadzone.hpp"
#include "services/interfaces/workflow/input/input_axis_source_value.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void CombineAndWriteAxis(const std::string& axisName,
                         const nlohmann::json& axisBinding,
                         WorkflowContext& context,
                         const nlohmann::json* keyState, bool gamepadConnected,
                         const std::shared_ptr<ILogger>& logger) {
    float accumulatedValue = 0.0f;

    for (const auto& source : axisBinding["sources"]) {
        if (!source.is_object() || !source.contains("type")) continue;

        float scale    = source.value("scale", 1.0f);
        bool invert    = source.value("invert", false);
        float deadzone = source.value("deadzone", 0.0f);

        float value =
            ReadAxisSourceValue(source, context, keyState, gamepadConnected);
        if (invert) value = -value;
        value = ApplyAxisDeadzone(value, deadzone);
        accumulatedValue += value * scale;
    }

    accumulatedValue = std::max(-1.0f, std::min(1.0f, accumulatedValue));

    if (axisBinding.contains("outputs") && axisBinding["outputs"].is_array()) {
        for (const auto& output : axisBinding["outputs"]) {
            if (output.is_string()) {
                context.Set<float>(output.get<std::string>(), accumulatedValue);
            }
        }
    }

    if (logger) {
        logger->Debug("input.axis.combine: '" + axisName +
                      "' = " + std::to_string(accumulatedValue));
    }
}

}  // namespace sdl3cpp::services::impl
