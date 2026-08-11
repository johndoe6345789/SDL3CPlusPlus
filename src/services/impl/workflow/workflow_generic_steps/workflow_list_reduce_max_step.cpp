#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_max_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListReduceMaxStep::WorkflowListReduceMaxStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListReduceMaxStep::GetPluginId() const {
    return "list.reduce.max";
}

void WorkflowListReduceMaxStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* list = context.TryGet<std::vector<double>>(listKey);
    if (!list || list->empty()) {
        throw std::runtime_error("list.reduce.max requires a non-empty numeric list");
    }

    const double result = *std::max_element(list->begin(), list->end());
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowListReduceMaxStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Reduced numeric list to max");
    }
}

}  // namespace sdl3cpp::services::impl
