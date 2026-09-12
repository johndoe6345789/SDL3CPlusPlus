#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.player.swim
 *
 * Into water deeper than the player's knees (water_file's quads) and they
 * swim: they float with the head out, W A S D swim at `speed` m/s along
 * the view (Left Shift faster), Space rises and climbs out, Left Ctrl
 * dives. No gravity; the world still stops them. Walking out onto a beach
 * gives the q3 move back. Runs after the q3 move and before
 * gta5.player.fly, which wins while flying. Publishes gta5.swimming.
 */
class WorkflowGta5PlayerSwimStep final : public IWorkflowStep {
public:
    WorkflowGta5PlayerSwimStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::vector<Gta5WaterQuad> water_;
    bool loaded_{false};
    bool swimming_{false};
    glm::vec3 last_{0.f};  // where the last swim move left the player
};

}  // namespace sdl3cpp::services::impl
