#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_join_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowStringJoinStep::WorkflowStringJoinStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringJoinStep::GetPluginId() const {
    return "string.join";
}

void WorkflowStringJoinStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string listKey = resolver.GetRequiredInputKey(step, "list");
    const std::string delimiterKey = resolver.GetRequiredInputKey(step, "delimiter");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* list = context.TryGet<std::vector<std::string>>(listKey);
    const auto* delimiter = context.TryGet<std::string>(delimiterKey);
    if (!list || !delimiter) {
        throw std::runtime_error("string.join requires list and delimiter inputs");
    }

    std::string result;
    for (size_t i = 0; i < list->size(); ++i) {
        if (i > 0) {
            result += *delimiter;
        }
        result += (*list)[i];
    }
    context.Set(outputKey, std::move(result));

    if (logger_) {
        logger_->Trace("WorkflowStringJoinStep", "Execute",
                       "input=" + listKey +
                           ", output=" + outputKey,
                       "Joined workflow strings");
    }
}

}  // namespace sdl3cpp::services::impl
