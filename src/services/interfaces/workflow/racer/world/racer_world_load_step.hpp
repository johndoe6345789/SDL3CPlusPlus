#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.world.load
 *
 * Opens an Episode I Racer install and loads one race from its own
 * data: the track's model (every mesh, material and texture, the
 * textures upscaled), its spline lap, and the chosen racer's pod.
 *
 * Parameters: `racer_dir` (env RACER_DIR), `track` (env RACER_TRACK; an
 * id 0-24 or part of a name, default Boonta Classic), `racer` (env
 * RACER_POD; part of a name, default Anakin), `track_table`,
 * `texture_scale` (power of two, default 4), `laps` (default 3),
 * `opponents` (default 7).
 * Publishes racer.track_name and racer.pod_name.
 */
class WorkflowRacerWorldLoadStep final : public IWorkflowStep {
public:
    WorkflowRacerWorldLoadStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
