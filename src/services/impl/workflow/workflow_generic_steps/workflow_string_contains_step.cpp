#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_contains_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringContainsStep::WorkflowStringContainsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringContainsStep::GetPluginId() const {
    return "string.contains";
}

void WorkflowStringContainsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string needleKey = resolver.GetRequiredInputKey(step, "needle");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<std::string>(valueKey);
    const auto* needle = context.TryGet<std::string>(needleKey);
    if (!value || !needle) {
        throw std::runtime_error("string.contains requires string inputs");
    }

    const bool result = value->find(*needle) != std::string::npos;
    context.Set(outputKey, result);

    if (logger_) {
        logger_->Trace("WorkflowStringContainsStep", "Execute",
                       "input=" + valueKey +
                           ", needle=" + needleKey +
                           ", output=" + outputKey,
                       "Checked workflow string containment");
    }
}

}  // namespace sdl3cpp::services::impl
