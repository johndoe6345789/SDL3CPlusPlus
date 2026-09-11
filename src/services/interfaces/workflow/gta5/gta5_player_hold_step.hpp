#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_roads.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.player.hold
 *
 * Holds the player where they spawned until the ground under them has
 * streamed in, and publishes a loading message while it waits.
 *
 * Read on demand, the ground takes a while: the asset index is built
 * first, then the spawn tile's thousands of placements are read and
 * uploaded. Unheld, the player falls through the empty world meanwhile
 * -- on a cold disk, far enough to leave the map. The rigid body and the
 * movement state (q3.ps) are both reset: the movement code keeps its own
 * velocity, which would otherwise build a fall speed through the hold
 * and release it all at once.
 *
 * Runs after the physics group. Writes gta5.loading.text, empty once the
 * ground is in. Releases after 120 s regardless, with a warning.
 */
class WorkflowGta5PlayerHoldStep final : public IWorkflowStep {
public:
    WorkflowGta5PlayerHoldStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    /// A double-click on the map (gta5.player.teleport_seq): hold there.
    /// True in the frame a trip starts.
    bool Travel(WorkflowContext& context, btRigidBody* player);
    /// Landed after a trip: the car follows, parked alongside.
    void Arrive(WorkflowContext& context, btRigidBody* player);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    glm::vec3 hold_{0.f};
    std::uint64_t startMs_{0};
    bool recorded_{false};
    bool released_{false};
    int travelSequence_{0};
    bool travelling_{false};
    Gta5Roads roads_;  // read on the first arrival, from roads_dir
    std::string roadsDir_;
};

}  // namespace sdl3cpp::services::impl
