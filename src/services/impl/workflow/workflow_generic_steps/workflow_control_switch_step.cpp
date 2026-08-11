#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_switch_step.hpp"

#include <any>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowControlSwitchStep::WorkflowControlSwitchStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)),
      registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error("WorkflowControlSwitchStep requires a step registry");
    }
}

std::string WorkflowControlSwitchStep::GetPluginId() const {
    return "control.condition.switch";
}

void WorkflowControlSwitchStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    // Get value key from inputs
    const auto valueIt = step.inputs.find("value");
    if (valueIt == step.inputs.end()) {
        throw std::runtime_error("control.condition.switch requires 'value' input");
    }

    const std::string& valueKey = valueIt->second;

    // Get value from context - can be string, number, or bool
    const auto* valueAny = context.TryGetAny(valueKey);
    if (!valueAny) {
        throw std::runtime_error("control.condition.switch: value key '" + valueKey +
                                 "' not found");
    }

    std::string valueStr;

    // Convert value to string for case matching
    if (const auto* strVal = std::any_cast<std::string>(valueAny)) {
        valueStr = *strVal;
    } else if (const auto* boolVal = std::any_cast<bool>(valueAny)) {
        valueStr = *boolVal ? "true" : "false";
    } else if (const auto* numVal = std::any_cast<double>(valueAny)) {
        valueStr = std::to_string(static_cast<long long>(*numVal));
    } else if (const auto* intVal = std::any_cast<int>(valueAny)) {
        valueStr = std::to_string(*intVal);
    } else {
        throw std::runtime_error(
            "control.condition.switch: value type must be string, bool, double, or int");
    }

    // Find matching case in inputs (cases are stored with "case_" prefix)
    std::string caseStepId;
    std::string defaultStepId;

    for (const auto& inputPair : step.inputs) {
        const std::string& key = inputPair.first;
        const std::string& value = inputPair.second;

        if (key == "value") {
            continue;  // Skip the value input itself
        }

        if (key == "default") {
            defaultStepId = value;
        } else if (key.substr(0, 5) == "case_") {
            const std::string& caseValue = key.substr(5);
            if (caseValue == valueStr) {
                caseStepId = value;
                break;
            }
        }
    }

    // Use matched case or default, or throw if neither found
    std::string selectedStepId = caseStepId.empty() ? defaultStepId : caseStepId;

    if (selectedStepId.empty()) {
        if (logger_) {
            logger_->Trace("WorkflowControlSwitchStep", "Execute",
                           "value=" + valueStr + ", no matching case and no default",
                           "No case matched");
        }
        return;
    }

    // Execute selected case step
    auto stepHandler = registry_->GetStep(selectedStepId);
    if (!stepHandler) {
        throw std::runtime_error("control.condition.switch: case step '" + selectedStepId +
                                 "' not found");
    }

    WorkflowStepDefinition caseStep;
    caseStep.plugin = selectedStepId;
    caseStep.id = selectedStepId;

    stepHandler->Execute(caseStep, context);

    if (logger_) {
        logger_->Trace("WorkflowControlSwitchStep", "Execute",
                       "value=" + valueStr + ", case=" + selectedStepId,
                       "Executed switch case");
    }
}

}  // namespace sdl3cpp::services::impl
