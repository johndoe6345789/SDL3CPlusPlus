#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.sw.end_upload_surface
 *
 * Uploads the SW overlay surface's pixels to the GPU overlay texture, once
 * per frame.
 */
class WorkflowOverlaySwEndUploadSurfaceStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndUploadSurfaceStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
