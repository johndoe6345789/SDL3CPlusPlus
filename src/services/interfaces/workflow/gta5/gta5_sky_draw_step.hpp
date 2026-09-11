#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.sky.draw
 *
 * Fills the frame with a procedural sky before the city draws over it.
 *
 * A fullscreen triangle at depth 1.0, so the geometry that follows
 * occludes it by ordinary depth test rather than by a second pass. The
 * colours come from the same sun direction and haze colour the model
 * shader fogs with: the horizon is where a mismatch shows.
 *
 * Reads:  gpu_render_pass, gpu_command_buffer, render.view_matrix,
 *         render.proj_matrix, render.camera_pos, render.frag_uniforms,
 *         frame_skip
 */
class WorkflowGta5SkyDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowGta5SkyDrawStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
