#include "services/interfaces/workflow/input/workflow_input_button_combine_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <fstream>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowInputButtonCombineStep::WorkflowInputButtonCombineStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputButtonCombineStep::GetPluginId() const {
    return "input.button.combine";
}

void WorkflowInputButtonCombineStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowInputButtonCombineStep", "Execute", "Entry");
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
            throw std::runtime_error("input.button.combine: Failed to open config: " + configPath);
        }
        configFile >> aggregationConfig;
    }

    if (!aggregationConfig.contains("inputBindings") ||
        !aggregationConfig["inputBindings"].contains("buttons")) {
        if (logger_) logger_->Debug("input.button.combine: No button bindings found, skipping");
        return;
    }

    // Read keyboard state and gamepad availability from context
    const auto* keyState = context.TryGet<nlohmann::json>("input.keyboard.state");
    bool gamepadConnected = context.Get<bool>("input.gamepad.connected", false);

    const auto& buttonsConfig = aggregationConfig["inputBindings"]["buttons"];
    for (auto it = buttonsConfig.begin(); it != buttonsConfig.end(); ++it) {
        const std::string& buttonName = it.key();
        const auto& buttonBinding = it.value();

        if (!buttonBinding.is_object() || !buttonBinding.contains("sources") ||
            !buttonBinding["sources"].is_array()) {
            continue;
        }

        bool pressed = false;

        for (const auto& source : buttonBinding["sources"]) {
            if (!source.is_object() || !source.contains("type")) continue;

            std::string sourceType = source["type"].get<std::string>();

            if (sourceType == "key") {
                std::string keyName = source.value("key", "");
                if (keyState && keyState->contains(keyName) && (*keyState)[keyName].get<bool>()) {
                    pressed = true;
                }
            } else if (sourceType == "mouse_button") {
                std::string btnStr = source.value("button", "");
                if (btnStr == "left") {
                    pressed = context.Get<bool>("input.mouse.left", false);
                } else if (btnStr == "right") {
                    pressed = context.Get<bool>("input.mouse.right", false);
                } else if (btnStr == "middle") {
                    pressed = context.Get<bool>("input.mouse.middle", false);
                }
            } else if (sourceType == "gamepad_button") {
                if (gamepadConnected) {
                    std::string btnStr = source.value("button", "");

                    // Map config button names to context keys
                    if (btnStr == "a") {
                        pressed = context.Get<bool>("input.gamepad.button_south", false);
                    } else if (btnStr == "b") {
                        pressed = context.Get<bool>("input.gamepad.button_east", false);
                    } else if (btnStr == "x") {
                        pressed = context.Get<bool>("input.gamepad.button_west", false);
                    } else if (btnStr == "y") {
                        pressed = context.Get<bool>("input.gamepad.button_north", false);
                    } else if (btnStr == "lb") {
                        pressed = context.Get<bool>("input.gamepad.button_left_shoulder", false);
                    } else if (btnStr == "rb") {
                        pressed = context.Get<bool>("input.gamepad.button_right_shoulder", false);
                    } else if (btnStr == "back") {
                        pressed = context.Get<bool>("input.gamepad.button_back", false);
                    } else if (btnStr == "start") {
                        pressed = context.Get<bool>("input.gamepad.button_start", false);
                    } else if (btnStr == "trigger_left") {
                        float threshold = source.value("threshold", 0.5f);
                        float triggerVal = context.Get<float>("input.gamepad.trigger_left", 0.0f);
                        pressed = (triggerVal >= threshold);
                    } else if (btnStr == "trigger_right") {
                        float threshold = source.value("threshold", 0.5f);
                        float triggerVal = context.Get<float>("input.gamepad.trigger_right", 0.0f);
                        pressed = (triggerVal >= threshold);
                    }
                }
            }

            if (pressed) break;  // Any source pressed = button pressed (OR logic)
        }

        // Write combined button state to configured output keys
        if (buttonBinding.contains("outputs") && buttonBinding["outputs"].is_array()) {
            for (const auto& output : buttonBinding["outputs"]) {
                if (output.is_string()) {
                    context.Set<bool>(output.get<std::string>(), pressed);
                }
            }
        }

        if (logger_ && pressed) {
            logger_->Debug("input.button.combine: '" + buttonName + "' = pressed");
        }
    }
}

}  // namespace sdl3cpp::services::impl
