#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_concat_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowListConcatStep::WorkflowListConcatStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowListConcatStep::GetPluginId() const {
    return "list.concat";
}

void WorkflowListConcatStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string leftKey = resolver.GetRequiredInputKey(step, "left");
    const std::string rightKey = resolver.GetRequiredInputKey(step, "right");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "list");

    if (const auto* left = context.TryGet<std::vector<double>>(leftKey)) {
        const auto* right = context.TryGet<std::vector<double>>(rightKey);
        if (!right) {
            throw std::runtime_error("list.concat requires both inputs to be numeric lists");
        }
        std::vector<double> result = *left;
        result.insert(result.end(), right->begin(), right->end());
        context.Set(outputKey, std::move(result));
    } else if (const auto* left = context.TryGet<std::vector<std::string>>(leftKey)) {
        const auto* right = context.TryGet<std::vector<std::string>>(rightKey);
        if (!right) {
            throw std::runtime_error("list.concat requires both inputs to be string lists");
        }
        std::vector<std::string> result = *left;
        result.insert(result.end(), right->begin(), right->end());
        context.Set(outputKey, std::move(result));
    } else {
        throw std::runtime_error("list.concat requires list inputs of strings or numbers");
    }

    if (logger_) {
        logger_->Trace("WorkflowListConcatStep", "Execute",
                       "left=" + leftKey +
                           ", right=" + rightKey +
                           ", output=" + outputKey,
                       "Concatenated workflow lists");
    }
}

}  // namespace sdl3cpp::services::impl
