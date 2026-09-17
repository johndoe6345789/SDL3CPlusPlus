#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Phase 1 of shader system initialization: selects the active
 * shader system.
 *
 * `shader.system.initialize` was split into three atomic, chained steps
 * (`shader.system.initialize`, `shader.system.gltf_load`,
 * `shader.system.compile`) so each phase is independently retryable and
 * the flat workflow executor can run them in sequence.
 *
 * Context Output:
 *   - shader.system.selected_id (std::string)
 *   - shader.system.selection_status (std::string) = "set"
 */
class WorkflowShaderSystemInitializeStep : public IWorkflowStep {
public:
    explicit WorkflowShaderSystemInitializeStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
