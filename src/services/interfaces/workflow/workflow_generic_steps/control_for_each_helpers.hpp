#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Runs `stepId` once per element of `items`, with `itemVarName`
 * (and `itemVarName + ".index"`) set in context for each iteration.
 *
 * Shared by control.loop.for_each's vector<std::string> and
 * vector<double> item-type branches, which are otherwise identical.
 */
template <typename T>
void RunForEachLoop(const std::vector<T>& items, const std::string& itemVarName,
                    const std::string& stepId,
                    const std::shared_ptr<IWorkflowStep>& stepHandler,
                    WorkflowContext& context) {
    int index = 0;
    for (const auto& item : items) {
        context.Set(itemVarName, item);
        context.Set(itemVarName + ".index", static_cast<double>(index));

        WorkflowStepDefinition loopStep;
        loopStep.plugin = stepId;
        loopStep.id     = stepId;
        stepHandler->Execute(loopStep, context);

        ++index;
    }
}

}  // namespace sdl3cpp::services::impl
