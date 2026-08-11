#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_min_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListReduceMinStep::WorkflowListReduceMinStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListReduceMinStep::GetPluginId() const {
    return "list.reduce.min";
}

void WorkflowListReduceMinStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* list = context.TryGet<std::vector<double>>(listKey);
    if (!list || list->empty()) {
        throw std::runtime_error("list.reduce.min requires a non-empty numeric list");
    }

    const double result = *std::min_element(list->begin(), list->end());
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowListReduceMinStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Reduced numeric list to min");
    }
}

}  // namespace sdl3cpp::services::impl
