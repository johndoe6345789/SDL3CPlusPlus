#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.fps_upload_quad
 *
 * Uploads the FPS overlay's static quad to its vertex buffer once, using
 * `frame_width`/`frame_height` (this sub-workflow's viewport-size keys).
 */
class WorkflowOverlayFpsUploadQuadStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlayFpsUploadQuadStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    bool uploaded_ = false;
};

}  // namespace sdl3cpp::services::impl
