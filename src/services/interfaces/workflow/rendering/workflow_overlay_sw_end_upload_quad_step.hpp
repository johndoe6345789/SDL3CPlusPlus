#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: overlay.sw.end_upload_quad
 *
 * Uploads the fixed fullscreen quad (clip-space corners, no viewport-relative
 * math — the overlay texture always covers the whole screen) once.
 */
class WorkflowOverlaySwEndUploadQuadStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndUploadQuadStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
