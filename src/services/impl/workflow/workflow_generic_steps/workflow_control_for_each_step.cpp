#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_for_each_step.hpp"

#include <any>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowControlForEachStep::WorkflowControlForEachStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)),
      registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error("WorkflowControlForEachStep requires a step registry");
    }
}

std::string WorkflowControlForEachStep::GetPluginId() const {
    return "control.loop.for_each";
}

void WorkflowControlForEachStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    // Get items array key from inputs
    const auto itemsIt = step.inputs.find("items");
    if (itemsIt == step.inputs.end()) {
        throw std::runtime_error("control.loop.for_each requires 'items' input");
    }

    const std::string& itemsKey = itemsIt->second;

    // Get item variable name from inputs
    const auto itemVarIt = step.inputs.find("item_var");
    if (itemVarIt == step.inputs.end()) {
        throw std::runtime_error("control.loop.for_each requires 'item_var' input");
    }

    const std::string& itemVarName = itemVarIt->second;

    // Get step to execute from inputs
    const auto stepIdIt = step.inputs.find("step_id");
    if (stepIdIt == step.inputs.end()) {
        throw std::runtime_error("control.loop.for_each requires 'step_id' input");
    }

    const std::string& stepId = stepIdIt->second;

    // Get items from context
    const auto* itemsAny = context.TryGetAny(itemsKey);
    if (!itemsAny) {
        throw std::runtime_error("control.loop.for_each: items key '" + itemsKey + "' not found");
    }

    // Try to handle as vector<std::string> (most common)
    const auto* stringVec = std::any_cast<std::vector<std::string>>(itemsAny);
    if (stringVec) {
        auto stepHandler = registry_->GetStep(stepId);
        if (!stepHandler) {
            throw std::runtime_error("control.loop.for_each: step '" + stepId + "' not found");
        }

        int index = 0;
        for (const auto& item : *stringVec) {
            // Set loop variables in context
            context.Set(itemVarName, item);
            context.Set(itemVarName + ".index", static_cast<double>(index));

            // Create step definition for loop body
            WorkflowStepDefinition loopStep;
            loopStep.plugin = stepId;
            loopStep.id = stepId;

            stepHandler->Execute(loopStep, context);

            index++;
        }

        if (logger_) {
            logger_->Trace("WorkflowControlForEachStep", "Execute",
                           "items=" + itemsKey + ", count=" + std::to_string(stringVec->size()) +
                               ", step=" + stepId,
                           "Completed for_each loop");
        }
        return;
    }

    // Try to handle as vector<double>
    const auto* numberVec = std::any_cast<std::vector<double>>(itemsAny);
    if (numberVec) {
        auto stepHandler = registry_->GetStep(stepId);
        if (!stepHandler) {
            throw std::runtime_error("control.loop.for_each: step '" + stepId + "' not found");
        }

        int index = 0;
        for (double item : *numberVec) {
            // Set loop variables in context
            context.Set(itemVarName, item);
            context.Set(itemVarName + ".index", static_cast<double>(index));

            // Create step definition for loop body
            WorkflowStepDefinition loopStep;
            loopStep.plugin = stepId;
            loopStep.id = stepId;

            stepHandler->Execute(loopStep, context);

            index++;
        }

        if (logger_) {
            logger_->Trace("WorkflowControlForEachStep", "Execute",
                           "items=" + itemsKey + ", count=" + std::to_string(numberVec->size()) +
                               ", step=" + stepId,
                           "Completed for_each loop");
        }
        return;
    }

    throw std::runtime_error(
        "control.loop.for_each: items must be vector<std::string> or vector<double>");
}

}  // namespace sdl3cpp::services::impl
