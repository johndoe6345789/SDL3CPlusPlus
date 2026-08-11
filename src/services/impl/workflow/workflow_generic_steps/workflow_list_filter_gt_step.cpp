#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_filter_gt_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListFilterGtStep::WorkflowListFilterGtStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListFilterGtStep::GetPluginId() const {
    return "list.filter.gt";
}

void WorkflowListFilterGtStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    const auto* list = context.TryGet<std::vector<double>>(listKey);
    const auto* value = context.TryGet<double>(valueKey);
    if (!list || !value) {
        throw std::runtime_error("list.filter.gt requires numeric list and value inputs");
    }

    std::vector<double> filtered;
    filtered.reserve(list->size());
    for (double entry : *list) {
        if (entry > *value) {
            filtered.push_back(entry);
        }
    }
    context.Set(outputKey, std::move(filtered));

    if (logger_) {
        logger_->Trace("WorkflowListFilterGtStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Filtered numeric list by greater-than");
    }
}

}  // namespace sdl3cpp::services::impl
