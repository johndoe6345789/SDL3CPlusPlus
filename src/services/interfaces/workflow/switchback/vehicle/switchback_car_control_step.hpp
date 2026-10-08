#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.car.control
 *
 * Keeps the player in the car: the first vehicle is always seated and
 * driven from the keyboard. There is no getting out and no on-foot body.
 *
 * Reads:  input.keyboard.state, physics_dt
 * Writes: gta5.vehicle.seated
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
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> vehicles_;
};

}  // namespace sdl3cpp::services::impl
