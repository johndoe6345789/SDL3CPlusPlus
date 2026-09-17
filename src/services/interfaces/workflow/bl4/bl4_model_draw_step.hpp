#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.models.draw
 *
 * Draws every resident tile's instances, one draw call per submesh per
 * instance (no instancing/culling yet -- see packages/bl4/README.md for
 * the scope this first slice covers).
 *
 * Parameters: pipeline_key (default gpu_pipeline_bl4_model), texture_key
 *             (default bl4_placeholder).
 * Reads: gpu_render_pass, gpu_command_buffer, render.view_matrix,
 *        render.proj_matrix, render.frag_uniforms, frame_skip
 */
class WorkflowBl4ModelDrawStep final : public IWorkflowStep {
public:
    WorkflowBl4ModelDrawStep(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
    bool warned_ = false;
};

}  // namespace sdl3cpp::services::impl
