#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws the FPS overlay quad over the composited swapchain image.
 *
 * Uses a second render pass with LOADOP_LOAD so the composite output underneath
 * is preserved.
 */
class WorkflowPostfxOverlayFpsDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxOverlayFpsDrawStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
