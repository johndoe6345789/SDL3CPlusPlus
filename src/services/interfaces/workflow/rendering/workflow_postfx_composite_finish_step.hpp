#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Retires a completed composite frame.
 *
 * Drops the swapchain texture the composite pass consumed and advances the
 * frame counter.  Only applies when `postfx.composite_draw` actually drew, so a
 * frame that bailed out early does not advance the counter.
 */
class WorkflowPostfxCompositeFinishStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxCompositeFinishStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
