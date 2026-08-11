#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_map_mul_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListMapMulStep::WorkflowListMapMulStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListMapMulStep::GetPluginId() const {
    return "list.map.mul";
}

void WorkflowListMapMulStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    const auto* list = context.TryGet<std::vector<double>>(listKey);
    const auto* value = context.TryGet<double>(valueKey);
    if (!list || !value) {
        throw std::runtime_error("list.map.mul requires numeric list and value inputs");
    }

    std::vector<double> mapped;
    mapped.reserve(list->size());
    for (double entry : *list) {
        mapped.push_back(entry * (*value));
    }
    context.Set(outputKey, std::move(mapped));

    if (logger_) {
        logger_->Trace("WorkflowListMapMulStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Scaled numeric list");
    }
}

}  // namespace sdl3cpp::services::impl
