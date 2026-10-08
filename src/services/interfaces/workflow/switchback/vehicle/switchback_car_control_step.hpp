#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_gearbox.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.car.control
 *
 * Keeps the player in the car: the first vehicle is always seated and
 * driven from the keyboard. There is no getting out and no on-foot body.
 *
 * Drives all four wheels so the car can reach about 300 mph, with traction
 * control and the gearbox's rev limiter, damps the suspension, and runs an
 * eight speed gearbox that publishes gta5.car.revs and gta5.car.gear for the
 * dials.
 *
 * Reads:  input.keyboard.state, physics_dt
 * Writes: gta5.vehicle.seated, gta5.car.revs, gta5.car.gear
 */
class WorkflowSwitchbackCarControlStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackCarControlStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Gta5StreamState> vehicles);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void TraceDrive(float speed, float revs, float traction);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> vehicles_;
    SwitchbackGearbox gearbox_;
    int framesSinceTrace_ = 0;
};

}  // namespace sdl3cpp::services::impl
