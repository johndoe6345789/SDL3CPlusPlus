#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.track.draw
 *
 * Draws the meshed track: the ground first, then the road surface,
 * both through the shared textured pipeline. Parameters `ground_key`
 * and `road_key` name the textures to bind, defaulting to whatever
 * the workflow loaded under "ground_texture" and "road_texture".
 */
class WorkflowStuntsTrackDrawStep final : public IWorkflowStep {
public:
    WorkflowStuntsTrackDrawStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<StuntsWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool traced_ = false;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<StuntsWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
