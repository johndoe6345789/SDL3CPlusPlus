#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.tiles.draw
 *
 * Draws every instance of every resident tile, one indexed draw per
 * instance against its archetype's shared GPU buffers.
 *
 * Reads:  gpu_render_pass, gpu_command_buffer, gpu_pipeline_textured,
 *         render.view_matrix, render.proj_matrix, render.camera_pos,
 *         render.shadow_vp, render.frag_uniforms, frame_skip
 * Writes: gta5.tiles.drawn_last_frame
 */
class WorkflowGta5TilesDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5TilesDrawStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl
