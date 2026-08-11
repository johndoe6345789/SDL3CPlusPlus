#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_append_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListAppendStep::WorkflowListAppendStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListAppendStep::GetPluginId() const {
    return "list.append";
}

void WorkflowListAppendStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    if (const auto* list = context.TryGet<std::vector<double>>(listKey)) {
        const auto* value = context.TryGet<double>(valueKey);
        if (!value) {
            throw std::runtime_error("list.append requires numeric value input");
        }
        std::vector<double> result = *list;
        result.push_back(*value);
        context.Set(outputKey, std::move(result));
    } else if (const auto* list = context.TryGet<std::vector<std::string>>(listKey)) {
        const auto* value = context.TryGet<std::string>(valueKey);
        if (!value) {
            throw std::runtime_error("list.append requires string value input");
        }
        std::vector<std::string> result = *list;
        result.push_back(*value);
        context.Set(outputKey, std::move(result));
    } else {
        throw std::runtime_error("list.append requires list input of strings or numbers");
    }

    if (logger_) {
        logger_->Trace("WorkflowListAppendStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Appended workflow list");
    }
}

}  // namespace sdl3cpp::services::impl
