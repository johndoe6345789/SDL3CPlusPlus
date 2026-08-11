#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_sum_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListReduceSumStep::WorkflowListReduceSumStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListReduceSumStep::GetPluginId() const {
    return "list.reduce.sum";
}

void WorkflowListReduceSumStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* list = context.TryGet<std::vector<double>>(listKey);
    if (!list) {
        throw std::runtime_error("list.reduce.sum missing numeric list input '" + listKey + "'");
    }

    double sum = 0.0;
    for (double entry : *list) {
        sum += entry;
    }
    context.Set(outputKey, sum);

    if (logger_) {
        logger_->Trace("WorkflowListReduceSumStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Reduced numeric list");
    }
}

}  // namespace sdl3cpp::services::impl
