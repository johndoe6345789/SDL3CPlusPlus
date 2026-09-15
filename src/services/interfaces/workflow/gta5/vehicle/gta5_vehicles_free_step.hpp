#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.vehicles.free
 *
 * Gives every car back at the end of a run: the player's and the
 * traffic's, out of the physics world and deleted. Nothing else does --
 * cars are only ever let go of one at a time, when the shop replaces
 * one or the traffic recycles one -- so whatever is still on the road
 * when the loop stops would otherwise outlive it, bodies, shapes,
 * motion states and raycasters alike.
 *
 * Runs after the frame loop and before the physics world goes, since
 * taking a body out of the world needs the world.
 */
class WorkflowGta5VehiclesFreeStep final : public IWorkflowStep {
public:
    WorkflowGta5VehiclesFreeStep(std::shared_ptr<ILogger> logger,
                                     std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl
