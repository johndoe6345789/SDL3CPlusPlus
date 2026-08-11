#include "services/interfaces/workflow/input/workflow_input_axis_combine_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <fstream>
#include <cmath>
#include <string>
#include <algorithm>

namespace sdl3cpp::services::impl {

WorkflowInputAxisCombineStep::WorkflowInputAxisCombineStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputAxisCombineStep::GetPluginId() const {
    return "input.axis.combine";
}

float WorkflowInputAxisCombineStep::ApplyDeadzone(float value, float deadzone) {
    float clamped = std::max(-1.0f, std::min(1.0f, value));
    if (std::abs(clamped) < deadzone) {
        return 0.0f;
    }
    if (clamped > 0.0f) {
        return (clamped - deadzone) / (1.0f - deadzone);
    }
    return (clamped + deadzone) / (1.0f - deadzone);
}

void WorkflowInputAxisCombineStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowInputAxisCombineStep", "Execute", "Entry");
    }

    // Load aggregation config from context or file
    WorkflowStepParameterResolver paramResolver;
    std::string configPath = "packages/seed/workflows/input_aggregation.json";
    if (const auto* param = paramResolver.FindParameter(step, "config_path")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            configPath = param->stringValue;
        }
    }

    const auto* contextConfig = context.TryGet<nlohmann::json>("input.aggregation.config");
    nlohmann::json aggregationConfig;

    if (contextConfig && contextConfig->is_object()) {
        aggregationConfig = *contextConfig;
    } else {
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            throw std::runtime_error("input.axis.combine: Failed to open config: " + configPath);
        }
        configFile >> aggregationConfig;
    }

    if (!aggregationConfig.contains("inputBindings") ||
        !aggregationConfig["inputBindings"].contains("axes")) {
        if (logger_) logger_->Debug("input.axis.combine: No axes bindings found, skipping");
        return;
    }

    // Read keyboard state written by input.keyboard.poll
    const auto* keyState = context.TryGet<nlohmann::json>("input.keyboard.state");
    bool gamepadConnected = context.Get<bool>("input.gamepad.connected", false);

    const auto& axesConfig = aggregationConfig["inputBindings"]["axes"];
    for (auto it = axesConfig.begin(); it != axesConfig.end(); ++it) {
        const std::string& axisName = it.key();
        const auto& axisBinding = it.value();

        if (!axisBinding.is_object() || !axisBinding.contains("sources") ||
            !axisBinding["sources"].is_array()) {
            continue;
        }

        float accumulatedValue = 0.0f;

        for (const auto& source : axisBinding["sources"]) {
            if (!source.is_object() || !source.contains("type")) continue;

            std::string sourceType = source["type"].get<std::string>();
            float scale = source.value("scale", 1.0f);
            bool invert = source.value("invert", false);
            float deadzone = source.value("deadzone", 0.0f);
            float value = 0.0f;

            if (sourceType == "key") {
                // Read from keyboard state in context
                std::string keyName = source.value("key", "");
                if (keyState && keyState->contains(keyName) && (*keyState)[keyName].get<bool>()) {
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

            if (invert) value = -value;
            value = ApplyDeadzone(value, deadzone);
            accumulatedValue += value * scale;
        }

        accumulatedValue = std::max(-1.0f, std::min(1.0f, accumulatedValue));

        // Write combined value to configured output keys
        if (axisBinding.contains("outputs") && axisBinding["outputs"].is_array()) {
            for (const auto& output : axisBinding["outputs"]) {
                if (output.is_string()) {
                    context.Set<float>(output.get<std::string>(), accumulatedValue);
                }
            }
        }

        if (logger_) {
            logger_->Debug("input.axis.combine: '" + axisName + "' = " +
                           std::to_string(accumulatedValue));
        }
    }
}

}  // namespace sdl3cpp::services::impl
