#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bsp.build_collision
 *
 * Builds Bullet collision shapes from the loaded BSP's brush lump: one
 * static compound body for solid brushes, and a second compound body
 * (CharacterFilter only) for player-clip brushes, matching Quake's
 * MASK_PLAYERSOLID / MASK_SHOT split.
 *
 * Reads from context: bsp_raw_data, bsp_config, physics_world.
 * Writes to context: bsp_collision_body, bsp_playerclip_body.
 */
class WorkflowBspBuildCollisionStep final : public IWorkflowStep {
public:
    explicit WorkflowBspBuildCollisionStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
