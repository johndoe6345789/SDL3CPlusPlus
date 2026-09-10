#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_log_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/debug_log_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowDebugLogStep::WorkflowDebugLogStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDebugLogStep::GetPluginId() const {
    return "debug.log";
}

void WorkflowDebugLogStep::Execute(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    WorkflowStepIoResolver ioResolver;
    const std::string messageKey =
        ioResolver.GetRequiredInputKey(step, "message");

    const auto* message = context.TryGet<std::string>(messageKey);
    if (!message) {
        throw std::runtime_error("debug.log missing input '" + messageKey +
                                 "'");
    }

    const DebugLogParams params = ReadDebugLogParams(step);
    const DebugLogLevel logLevel = ParseDebugLogLevel(params.level);
    EmitDebugLog(logger_, logLevel, params.contextLabel, *message);
}

}  // namespace sdl3cpp::services::impl
