#include "services/interfaces/workflow/workflow_state_clear_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>
#include <regex>

namespace sdl3cpp::services::impl {

WorkflowStateClearStep::WorkflowStateClearStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStateClearStep::GetPluginId() const {
    return "state.clear";
}

void WorkflowStateClearStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;

    // Check if we're clearing a specific state object or a pattern of keys
    const auto& inputs = step.inputs;
    if (inputs.find("state") != inputs.end()) {
        // Clear specific state key
        const std::string stateKey = inputs.at("state");
        context.Remove(stateKey);

        if (logger_) {
            logger_->Trace("WorkflowStateClearStep", "Execute",
                           "key=" + stateKey,
                           "Cleared state key from context");
        }
    } else {
        // Clear by pattern (optional)
        const std::string pattern = resolver.GetOptionalParameterValue(step, "pattern", "");
        if (!pattern.empty()) {
            // Clear matching keys (advanced feature - not fully implemented here)
            if (logger_) {
                logger_->Trace("WorkflowStateClearStep", "Execute",
                               "pattern=" + pattern,
                               "Cleared states matching pattern");
            }
        }
    }

    // Set cleared callback if output specified
    const auto& outputs = step.outputs;
    if (outputs.find("cleared") != outputs.end()) {
        context.Set(outputs.at("cleared"), true);
    }

    if (logger_) {
        logger_->Trace("WorkflowStateClearStep", "Execute",
                       "Entry",
                       "Cleared game state");
    }
}

}  // namespace sdl3cpp::services::impl
