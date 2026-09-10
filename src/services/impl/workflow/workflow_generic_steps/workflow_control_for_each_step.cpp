#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_for_each_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/control_for_each_helpers.hpp"

#include <any>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowControlForEachStep::WorkflowControlForEachStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)), registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error(
            "WorkflowControlForEachStep requires a step registry");
    }
}

std::string WorkflowControlForEachStep::GetPluginId() const {
    return "control.loop.for_each";
}

void WorkflowControlForEachStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    auto requireInput = [&](const char* name) -> const std::string& {
        const auto it = step.inputs.find(name);
        if (it == step.inputs.end()) {
            throw std::runtime_error(
                "control.loop.for_each requires '" + std::string(name) +
                "' input");
        }
        return it->second;
    };
    const std::string& itemsKey    = requireInput("items");
    const std::string& itemVarName = requireInput("item_var");
    const std::string& stepId      = requireInput("step_id");

    const auto* itemsAny = context.TryGetAny(itemsKey);
    if (!itemsAny) {
        throw std::runtime_error("control.loop.for_each: items key '" +
                                 itemsKey + "' not found");
    }

    const auto* stringVec =
        std::any_cast<std::vector<std::string>>(itemsAny);
    const auto* numberVec = std::any_cast<std::vector<double>>(itemsAny);
    if (!stringVec && !numberVec) {
        throw std::runtime_error(
            "control.loop.for_each: items must be vector<std::string> or "
            "vector<double>");
    }

    auto stepHandler = registry_->GetStep(stepId);
    if (!stepHandler) {
        throw std::runtime_error("control.loop.for_each: step '" + stepId +
                                 "' not found");
    }

    const size_t count = stringVec ? stringVec->size() : numberVec->size();
    if (stringVec) {
        RunForEachLoop(*stringVec, itemVarName, stepId, stepHandler, context);
    } else {
        RunForEachLoop(*numberVec, itemVarName, stepId, stepHandler, context);
    }

    if (logger_) {
        logger_->Trace(
            "WorkflowControlForEachStep", "Execute",
            "items=" + itemsKey + ", count=" + std::to_string(count) +
                ", step=" + stepId,
            "Completed for_each loop");
    }
}

}  // namespace sdl3cpp::services::impl
