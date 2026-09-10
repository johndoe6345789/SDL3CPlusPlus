#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: draw.textured_box
 *
 * Draws an axis-aligned box as 6 textured quads on the shared unit-plane
 * mesh, using pre-computed camera/lighting/shadow state from context and
 * (optionally) a physics body's synced transform for its pose.
 *
 * Reads from context: plane_unit_vb/ib/plane_unit, <texture>_gpu/_sampler,
 * render.view_matrix, render.proj_matrix, render.camera_pos,
 * render.frag_uniforms, render.shadow_vp, body_sync_<body>,
 * gpu_render_pass, gpu_command_buffer, gpu_pipeline_textured.
 */
class WorkflowDrawTexturedBoxStep final : public IWorkflowStep {
public:
    explicit WorkflowDrawTexturedBoxStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
