#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.vehicle.spawn
 *
 * Drops one vehicle into the world as a dynamic Bullet body, once.
 *
 * Runs inside the frame loop but spawns only on its first execution:
 * the streamer has to have put a road underneath before there is
 * anything to land on, so this cannot happen during setup.
 *
 * Reads:  gpu_device, physics_world
 * Writes: nothing; the vehicle joins the state vehicle list
 */
class WorkflowGta5VehicleSpawnStep final : public IWorkflowStep {
public:
    WorkflowGta5VehicleSpawnStep(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool spawned_{false};
};

}  // namespace sdl3cpp::services::impl
