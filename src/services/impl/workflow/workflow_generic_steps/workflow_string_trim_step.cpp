#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_trim_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringTrimStep::WorkflowStringTrimStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringTrimStep::GetPluginId() const {
    return "string.trim";
}

void WorkflowStringTrimStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<std::string>(valueKey);
    if (!value) {
        throw std::runtime_error("string.trim requires string input");
    }

    size_t start = 0;
    size_t end = value->size();
    while (start < end && std::isspace(static_cast<unsigned char>((*value)[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>((*value)[end - 1]))) {
        --end;
    }
    context.Set(outputKey, value->substr(start, end - start));

    if (logger_) {
        logger_->Trace("WorkflowStringTrimStep", "Execute",
                       "input=" + valueKey +
                           ", output=" + outputKey,
                       "Trimmed workflow string");
    }
}

}  // namespace sdl3cpp::services::impl
