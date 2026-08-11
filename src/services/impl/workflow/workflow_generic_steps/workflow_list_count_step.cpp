#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_count_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListCountStep::WorkflowListCountStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListCountStep::GetPluginId() const {
    return "list.count";
}

void WorkflowListCountStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    if (const auto* list = context.TryGet<std::vector<double>>(listKey)) {
        context.Set(outputKey, static_cast<double>(list->size()));
    } else if (const auto* list = context.TryGet<std::vector<std::string>>(listKey)) {
        context.Set(outputKey, static_cast<double>(list->size()));
    } else {
        throw std::runtime_error("list.count requires list input of strings or numbers");
    }

    if (logger_) {
        logger_->Trace("WorkflowListCountStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Counted workflow list");
    }
}

}  // namespace sdl3cpp::services::impl
