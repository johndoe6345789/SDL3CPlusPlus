#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_replace_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStringReplaceStep::WorkflowStringReplaceStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringReplaceStep::GetPluginId() const {
    return "string.replace";
}

void WorkflowStringReplaceStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string searchKey = resolver.GetRequiredInputKey(step, "search");
    const std::string replaceKey = resolver.GetRequiredInputKey(step, "replace");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* value = context.TryGet<std::string>(valueKey);
    const auto* search = context.TryGet<std::string>(searchKey);
    const auto* replace = context.TryGet<std::string>(replaceKey);
    if (!value || !search || !replace) {
        throw std::runtime_error("string.replace requires string inputs");
    }

    if (search->empty()) {
        throw std::runtime_error("string.replace search string cannot be empty");
    }

    std::string result = *value;
    size_t pos = 0;
    while ((pos = result.find(*search, pos)) != std::string::npos) {
        result.replace(pos, search->size(), *replace);
        pos += replace->size();
    }

    context.Set(outputKey, std::move(result));

    if (logger_) {
        logger_->Trace("WorkflowStringReplaceStep", "Execute",
                       "input=" + valueKey +
                           ", output=" + outputKey,
                       "Replaced workflow string content");
    }
}

}  // namespace sdl3cpp::services::impl
