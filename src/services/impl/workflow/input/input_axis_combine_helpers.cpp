#include "services/interfaces/workflow/input/input_axis_combine_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

float ApplyAxisDeadzone(float value, float deadzone) {
    float clamped = std::max(-1.0f, std::min(1.0f, value));
    if (std::abs(clamped) < deadzone) {
        return 0.0f;
    }
    if (clamped > 0.0f) {
        return (clamped - deadzone) / (1.0f - deadzone);
    }
    return (clamped + deadzone) / (1.0f - deadzone);
}

nlohmann::json LoadInputAggregationConfig(const WorkflowStepDefinition& step,
                                          const WorkflowContext& context) {
    WorkflowStepParameterResolver paramResolver;
    std::string configPath = "packages/seed/workflows/input_aggregation.json";
    if (const auto* param =
            paramResolver.FindParameter(step, "config_path")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            configPath = param->stringValue;
        }
    }

    const auto* contextConfig =
        context.TryGet<nlohmann::json>("input.aggregation.config");
    if (contextConfig && contextConfig->is_object()) {
        return *contextConfig;
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error(
            "input.axis.combine: Failed to open config: " + configPath);
    }
    nlohmann::json aggregationConfig;
    configFile >> aggregationConfig;
    return aggregationConfig;
}

float ReadAxisSourceValue(const nlohmann::json& source,
                          const WorkflowContext& context,
                          const nlohmann::json* keyState,
                          bool gamepadConnected) {
    const std::string sourceType = source["type"].get<std::string>();
    float value = 0.0f;

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
            std::string axisStr = source.value("axis", "");
            std::string contextKey = "input.gamepad." + axisStr;
            value = context.Get<float>(contextKey, 0.0f);
        }
    }
    return value;
}

void CombineAndWriteAxis(const std::string& axisName,
                         const nlohmann::json& axisBinding,
                         WorkflowContext& context,
                         const nlohmann::json* keyState,
                         bool gamepadConnected,
                         const std::shared_ptr<ILogger>& logger) {
    float accumulatedValue = 0.0f;

    for (const auto& source : axisBinding["sources"]) {
        if (!source.is_object() || !source.contains("type")) continue;

        float scale = source.value("scale", 1.0f);
        bool invert = source.value("invert", false);
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
        logger->Debug("input.axis.combine: '" + axisName + "' = " +
                     std::to_string(accumulatedValue));
    }
}

}  // namespace sdl3cpp::services::impl
