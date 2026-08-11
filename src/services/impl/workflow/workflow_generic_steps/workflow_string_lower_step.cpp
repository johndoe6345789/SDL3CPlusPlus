#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_lower_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringLowerStep::WorkflowStringLowerStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringLowerStep::GetPluginId() const {
    return "string.lower";
}

void WorkflowStringLowerStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<std::string>(valueKey);
    if (!value) {
        throw std::runtime_error("string.lower requires string input");
    }

    std::string result = *value;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    context.Set(outputKey, std::move(result));

    if (logger_) {
        logger_->Trace("WorkflowStringLowerStep", "Execute",
                       "input=" + valueKey +
                           ", output=" + outputKey,
                       "Lowercased workflow string");
    }
}

}  // namespace sdl3cpp::services::impl
