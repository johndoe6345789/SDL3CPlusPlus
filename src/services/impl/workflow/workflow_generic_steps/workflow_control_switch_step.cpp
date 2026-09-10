#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_switch_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/switch_value_matcher.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowControlSwitchStep::WorkflowControlSwitchStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)), registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error(
            "WorkflowControlSwitchStep requires a step registry");
    }
}

std::string WorkflowControlSwitchStep::GetPluginId() const {
    return "control.condition.switch";
}

void WorkflowControlSwitchStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    const auto valueIt = step.inputs.find("value");
    if (valueIt == step.inputs.end()) {
        throw std::runtime_error(
            "control.condition.switch requires 'value' input");
    }
    const std::string& valueKey = valueIt->second;

    const auto* valueAny = context.TryGetAny(valueKey);
    if (!valueAny) {
        throw std::runtime_error("control.condition.switch: value key '" +
                                 valueKey + "' not found");
    }

    const std::string valueStr = SwitchValueToString(valueAny);
    const std::string selectedStepId =
        FindSwitchCaseStepId(step.inputs, valueStr);

    if (selectedStepId.empty()) {
        if (logger_) {
            logger_->Trace(
                "WorkflowControlSwitchStep", "Execute",
                "value=" + valueStr + ", no matching case and no default",
                "No case matched");
        }
        return;
    }

    auto stepHandler = registry_->GetStep(selectedStepId);
    if (!stepHandler) {
        throw std::runtime_error("control.condition.switch: case step '" +
                                 selectedStepId + "' not found");
    }

    WorkflowStepDefinition caseStep;
    caseStep.plugin = selectedStepId;
    caseStep.id     = selectedStepId;
    stepHandler->Execute(caseStep, context);

    if (logger_) {
        logger_->Trace("WorkflowControlSwitchStep", "Execute",
                       "value=" + valueStr + ", case=" + selectedStepId,
                       "Executed switch case");
    }
}

}  // namespace sdl3cpp::services::impl
