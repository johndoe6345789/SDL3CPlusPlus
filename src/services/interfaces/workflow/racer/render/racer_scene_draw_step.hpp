#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Uniforms for racer_model.vert (std140).
struct RacerVertexUniforms {
    glm::mat4 viewProj;
    glm::mat4 model;
};

/// Uniforms for racer_model.frag (std140).
struct RacerFragmentUniforms {
    glm::vec4 fogColour;   ///< rgb, a = distance where fog starts
    glm::vec4 fogParams;   ///< x = distance of full fog, y = alpha cutoff
    glm::vec4 cameraPos;
};

/**
 * Plugin ID: racer.scene.draw
 *
 * Draws the track, then the player's pod at its pose: opaque batches
 * with gpu_pipeline_racer, then the intensity decals (shadows, glows)
 * with gpu_pipeline_racer_blend. Parameters: fog_r/g/b, fog_start,
 * fog_end (engine units).
 */
class WorkflowRacerSceneDrawStep final : public IWorkflowStep {
public:
    WorkflowRacerSceneDrawStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    bool traced_ = false;
};

}  // namespace sdl3cpp::services::impl
