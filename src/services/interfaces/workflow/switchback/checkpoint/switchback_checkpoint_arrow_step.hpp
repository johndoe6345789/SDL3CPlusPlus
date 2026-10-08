#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_route.hpp"
#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.checkpoint.arrow
 *
 * Passes checkpoints as the seated car reaches them, draws a race gantry at
 * every checkpoint, and draws a 3D arrow above the car pointing at the next
 * one. The gantries stay up after the race; the arrow stops at the finish.
 * Off the route (menu or free roam) nothing is drawn.
 *
 * The checkpoints come from the loaded track (switchback.terrain.load).
 * Parameters: radius (default 14),
 *             pipeline_key (default gpu_pipeline_arrow),
 *             marquee_pipeline_key (default gpu_pipeline_marquee)
 * Reads:      gpu_render_pass, gpu_command_buffer, gpu_device,
 *             render.view_matrix, render.proj_matrix, render.camera_pos,
 *             camera.state
 */
class WorkflowSwitchbackCheckpointArrowStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackCheckpointArrowStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Gta5StreamState> vehicles,
        std::shared_ptr<SwitchbackTerrainState> terrain,
        std::shared_ptr<SwitchbackSession> session);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void LoadOnce(const WorkflowStepDefinition& step, WorkflowContext& context);
    void LoadMarquees(SDL_GPUDevice* device);
    void DrawMarquees(const WorkflowStepDefinition& step,
                      WorkflowContext& context);
    void DrawArrow(const WorkflowStepDefinition& step,
                   WorkflowContext& context, const glm::vec3& car);
    /// Publishes the checkpoints passed, the total and the finish flag for
    /// the dash.
    void PublishProgress(WorkflowContext& context) const;

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> vehicles_;
    std::shared_ptr<SwitchbackTerrainState> terrain_;
    std::shared_ptr<SwitchbackSession> session_;
    SwitchbackCheckpointRoute route_;
    GeometryPlaneBuffers arrow_;
    std::uint32_t arrowIndexCount_ = 0;
    GeometryPlaneBuffers marquee_;
    std::uint32_t marqueeIndexCount_ = 0;
    float radius_ = 14.f;
    bool loaded_ = false;
};

}  // namespace sdl3cpp::services::impl
