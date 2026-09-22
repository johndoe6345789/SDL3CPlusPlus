#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/stunts/player/stunts_car_physics.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.car.drive
 *
 * Drives the loaded car from `input.move_forward` (throttle and brake)
 * and `input.move_right` (steering), using the gear count and rev
 * range read out of that car's own .RES. The grid the track was built
 * from decides what is under the wheels, so leaving the road costs
 * grip and speed the way it always has.
 *
 * Publishes stunts.car_pos, stunts.car_heading, stunts.speed_mph,
 * stunts.rpm and stunts.gear, and places the camera behind the car.
 */
class WorkflowStuntsCarDriveStep final : public IWorkflowStep {
public:
    WorkflowStuntsCarDriveStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<StuntsWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    StuntsCarState car_;
    bool placed_ = false;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<StuntsWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
