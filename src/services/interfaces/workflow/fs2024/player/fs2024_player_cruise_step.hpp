#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.player.cruise
 *
 * Flies the player straight along the spawn heading at `speed` m/s,
 * `height` metres above whatever ground has streamed in -- a hands-off
 * route for recording a flight over streaming scenery (the free-flight
 * keys need the window focused). Does nothing while speed is 0 or
 * empty, so it can sit in the physics workflow unused.
 *
 * Parameters: speed (m/s), height (metres, default 300).
 * Reads: physics_dt. Writes: q3.ps, q3.player_pos
 */
class WorkflowFs2024PlayerCruiseStep final : public IWorkflowStep {
public:
    WorkflowFs2024PlayerCruiseStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
    bool started_ = false;
    glm::vec3 at_{0.f};
    float reportIn_ = 0.f;
};

}  // namespace sdl3cpp::services::impl
