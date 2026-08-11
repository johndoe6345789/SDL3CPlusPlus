#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_map_add_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListMapAddStep::WorkflowListMapAddStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListMapAddStep::GetPluginId() const {
    return "list.map.add";
}

void WorkflowListMapAddStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    const auto* list = context.TryGet<std::vector<double>>(listKey);
    if (!list) {
        throw std::runtime_error("list.map.add missing numeric list input '" + listKey + "'");
    }
    const auto* value = context.TryGet<double>(valueKey);
    if (!value) {
        throw std::runtime_error("list.map.add missing numeric value input '" + valueKey + "'");
    }

    std::vector<double> mapped;
    mapped.reserve(list->size());
    for (double entry : *list) {
        mapped.push_back(entry + *value);
    }
    context.Set(outputKey, std::move(mapped));

    if (logger_) {
        logger_->Trace("WorkflowListMapAddStep", "Execute",
                       "input=" + listKey +
                           ", add=" + std::to_string(*value) +
                           ", output=" + outputKey,
                       "Mapped numeric list");
    }
}

}  // namespace sdl3cpp::services::impl
