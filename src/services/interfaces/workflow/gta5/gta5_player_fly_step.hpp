#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.player.fly
 *
 * Y toggles flying on foot. W flies where the camera looks, S backs off,
 * A and D strafe, Space rises and Left Ctrl sinks. There is no gravity,
 * but the world still blocks the player, who slides along it. The longer
 * W is held the faster it goes -- base_speed times e to the power of
 * seconds / ramp_seconds, up to max_speed -- and letting go of W drops
 * back to base_speed. Runs after the q3 move, which it replaces while
 * flying.
 */
class WorkflowGta5PlayerFlyStep final : public IWorkflowStep {
public:
    WorkflowGta5PlayerFlyStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool flying_{false};
    bool held_{false};
    float heldFor_{0.f};   // seconds W has been down, this run
    float reportIn_{0.f};  // seconds to the next speed line
    glm::vec3 last_{0.f};  // where the last fly move left the player
};

}  // namespace sdl3cpp::services::impl
