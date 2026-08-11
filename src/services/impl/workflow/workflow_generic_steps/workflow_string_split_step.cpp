#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_split_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowStringSplitStep::WorkflowStringSplitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStringSplitStep::GetPluginId() const {
    return "string.split";
}

void WorkflowStringSplitStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const std::string delimiterKey = resolver.GetRequiredInputKey(step, "delimiter");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    const auto* value = context.TryGet<std::string>(valueKey);
    const auto* delimiter = context.TryGet<std::string>(delimiterKey);
    if (!value || !delimiter) {
        throw std::runtime_error("string.split requires string inputs");
    }
    if (delimiter->empty()) {
        throw std::runtime_error("string.split delimiter cannot be empty");
    }

    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = 0;
    while ((pos = value->find(*delimiter, start)) != std::string::npos) {
        parts.emplace_back(value->substr(start, pos - start));
        start = pos + delimiter->size();
    }
    parts.emplace_back(value->substr(start));
    context.Set(outputKey, std::move(parts));

    if (logger_) {
        logger_->Trace("WorkflowStringSplitStep", "Execute",
                       "input=" + valueKey +
                           ", output=" + outputKey,
                       "Split workflow string");
    }
}

}  // namespace sdl3cpp::services::impl
