#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_climb.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.player.climb
 *
 * Space facing a wall with a top 0.4-2 m up climbs it, as GTA's vault:
 * the body rises, then carries over onto the top, in about half a
 * second, and nothing else moves it meanwhile. Anywhere else Space is
 * the q3 jump. After the q3 move, whose jump it overrides.
 */
class WorkflowGta5PlayerClimbStep final : public IWorkflowStep {
public:
    WorkflowGta5PlayerClimbStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Begin(WorkflowContext& context, const glm::vec3& origin,
               const glm::vec3& feet, float height);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5Climb climb_;
    bool held_{false};
};

}  // namespace sdl3cpp::services::impl
