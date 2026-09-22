#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.dashboard.draw
 *
 * Draws the cockpit dashboard (see stunts_dashboard_mesh.hpp) only
 * when `stunts.camera_mode` is "cockpit", rebuilding its small mesh
 * fresh each frame from `stunts.speed_mph`/`stunts.rpm` since the
 * needles move. Cheap enough (well under a thousand vertices) that
 * rebuilding beats tracking a dirty flag.
 *
 * Parameters: `speed_max_mph` (gauge range), `redline_rpm`.
 */
class WorkflowStuntsDashboardDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowStuntsDashboardDrawStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    SDL_GPUBuffer* vertexBuffer_ = nullptr;
    SDL_GPUBuffer* indexBuffer_ = nullptr;
    std::uint32_t indexCount_ = 0;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
