#include "services/interfaces/workflow/workflow_cmdline_args_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

WorkflowCmdlineArgsStep::WorkflowCmdlineArgsStep(std::shared_ptr<ILogger> logger)
    : logger_(logger) {}

std::string WorkflowCmdlineArgsStep::GetPluginId() const {
    return "system.cmdline";
}

void WorkflowCmdlineArgsStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!logger_) {
        return;
    }

    logger_->Trace("WorkflowCmdlineArgsStep", "Execute", "Parsing command line arguments");

    // Get argc and argv from parameters or context
    int argc = 0;
    auto it = step.parameters.find("argc");
    if (it != step.parameters.end()) {
        argc = static_cast<int>(it->second.numberValue);
    }

    // Store in context
    context.Set<int>("cmdline.argc", argc);

    logger_->Trace("WorkflowCmdlineArgsStep", "Execute",
                   "argc=" + std::to_string(argc),
                   "Command line arguments parsed");
}

}  // namespace sdl3cpp::services::impl
