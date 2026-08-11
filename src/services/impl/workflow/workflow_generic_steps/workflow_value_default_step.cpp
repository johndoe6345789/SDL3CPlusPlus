#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_default_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowValueDefaultStep::WorkflowValueDefaultStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowValueDefaultStep::GetPluginId() const {
    return "value.default";
}

void WorkflowValueDefaultStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string primaryKey = resolver.GetRequiredInputKey(step, "primary");
    const std::string fallbackKey = resolver.GetRequiredInputKey(step, "fallback");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "value");

    const auto* primary = context.TryGetAny(primaryKey);
    const auto* fallback = context.TryGetAny(fallbackKey);
    if (!primary && !fallback) {
        throw std::runtime_error("value.default missing inputs '" + primaryKey + "' and '" + fallbackKey + "'");
    }

    const char* source = primary ? "primary" : "fallback";
    context.Set(outputKey, primary ? *primary : *fallback);

    if (logger_) {
        logger_->Trace("WorkflowValueDefaultStep", "Execute",
                       "source=" + std::string(source) +
                           ", output=" + outputKey,
                       "Selected default workflow value");
    }
}

}  // namespace sdl3cpp::services::impl
