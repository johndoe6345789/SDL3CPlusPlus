#include "services/interfaces/workflow/input/workflow_input_button_combine_step.hpp"
#include "services/interfaces/workflow/input/input_button_combine_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>

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

    WorkflowStepParameterResolver paramResolver;
    std::string configPath = "packages/seed/workflows/input_aggregation.json";
    if (const auto* param =
            paramResolver.FindParameter(step, "config_path")) {
        if (param->type == WorkflowParameterValue::Type::String) {
            configPath = param->stringValue;
        }
    }

    const nlohmann::json aggregationConfig =
        LoadButtonAggregationConfig(context, configPath);
    if (!aggregationConfig.contains("inputBindings") ||
        !aggregationConfig["inputBindings"].contains("buttons")) {
        if (logger_) {
            logger_->Debug(
                "input.button.combine: No button bindings found, skipping");
        }
        return;
    }

    const auto* keyState =
        context.TryGet<nlohmann::json>("input.keyboard.state");
    const bool gamepadConnected =
        context.Get<bool>("input.gamepad.connected", false);

    const auto& buttonsConfig = aggregationConfig["inputBindings"]["buttons"];
    for (auto it = buttonsConfig.begin(); it != buttonsConfig.end(); ++it) {
        const std::string& buttonName = it.key();
        const auto& buttonBinding     = it.value();
        if (!buttonBinding.is_object() || !buttonBinding.contains("sources") ||
            !buttonBinding["sources"].is_array()) {
            continue;
        }

        bool pressed = false;
        for (const auto& source : buttonBinding["sources"]) {
            if (IsSourcePressed(source, keyState, gamepadConnected,
                               context)) {
                pressed = true;
                break;  // Any source pressed = button pressed (OR logic)
            }
        }

        WriteButtonOutputs(buttonBinding, pressed, context);

        if (logger_ && pressed) {
            logger_->Debug("input.button.combine: '" + buttonName +
                           "' = pressed");
        }
    }
}

}  // namespace sdl3cpp::services::impl
