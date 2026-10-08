#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.terrain.draw
 *
 * Draws every terrain chunk through the textured pipeline, binding the
 * texture named by `texture_key` (default "switchback_ground").
 *
 * Parameters: pipeline_key (default gpu_pipeline_textured), texture_key
 * Reads:      gpu_render_pass, gpu_command_buffer, render.view_matrix,
 *             render.proj_matrix, render.camera_pos, render.frag_uniforms
 */
class WorkflowSwitchbackTerrainDrawStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackTerrainDrawStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SwitchbackTerrainState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<SwitchbackTerrainState> state_;
};

}  // namespace sdl3cpp::services::impl
