#include "services/interfaces/workflow/input/workflow_input_axis_combine_step.hpp"
#include "services/interfaces/workflow/input/input_aggregation_config.hpp"
#include "services/interfaces/workflow/input/input_axis_combine_write.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowInputAxisCombineStep::WorkflowInputAxisCombineStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputAxisCombineStep::GetPluginId() const {
    return "input.axis.combine";
}

void WorkflowInputAxisCombineStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowInputAxisCombineStep", "Execute", "Entry");
    }

    const nlohmann::json aggregationConfig =
        LoadInputAggregationConfig(step, context);

    if (!aggregationConfig.contains("inputBindings") ||
        !aggregationConfig["inputBindings"].contains("axes")) {
        if (logger_) {
            logger_->Debug(
                "input.axis.combine: No axes bindings found, skipping");
        }
        return;
    }

    // Read keyboard state written by input.keyboard.poll.
    const auto* keyState =
        context.TryGet<nlohmann::json>("input.keyboard.state");
    const bool gamepadConnected =
        context.Get<bool>("input.gamepad.connected", false);

    const auto& axesConfig = aggregationConfig["inputBindings"]["axes"];
    for (auto it = axesConfig.begin(); it != axesConfig.end(); ++it) {
        const std::string& axisName = it.key();
        const auto& axisBinding     = it.value();

        if (!axisBinding.is_object() || !axisBinding.contains("sources") ||
            !axisBinding["sources"].is_array()) {
            continue;
        }

        CombineAndWriteAxis(axisName, axisBinding, context, keyState,
                            gamepadConnected, logger_);
    }
}

}  // namespace sdl3cpp::services::impl
