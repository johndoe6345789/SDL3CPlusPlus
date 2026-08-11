#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Poll SDL keyboard state and write key states to context.
 *
 * Plugin ID: "input.keyboard.poll"
 *
 * Reads the full SDL keyboard state array and stores it in context
 * as a JSON object mapping SDL scancode names to booleans.
 *
 * Context Output:
 *   - input.keyboard.state (nlohmann::json object): Map of key name -> pressed bool
 *   - input.keyboard.num_keys (int): Number of keys in the keyboard state array
 */
class WorkflowInputKeyboardPollStep final : public IWorkflowStep {
public:
    explicit WorkflowInputKeyboardPollStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
