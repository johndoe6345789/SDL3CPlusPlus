#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Uploads the FPS overlay's static quad to its vertex buffer once.
 *
 * The quad's position depends only on the viewport size, so it is written on
 * the first frame the overlay is available and left alone afterwards.
 */
class WorkflowPostfxOverlayFpsUploadQuadStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxOverlayFpsUploadQuadStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    bool uploaded_ = false;
};

}  // namespace sdl3cpp::services::impl
