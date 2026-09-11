#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.vehicle.control
 *
 * F gets in and out of the nearest car; W/S/A/D drive it and space
 * brakes.
 *
 * While seated the player's physics body is pinned to the chassis each
 * frame, which is what carries the camera along: the camera follows the
 * player body, so moving the body is enough and no separate camera mode
 * is needed.
 *
 * Reads:  input.keyboard.state, physics_body_<player>
 * Writes: gta5.vehicle.seated
 */
class WorkflowGta5VehicleControlStep final : public IWorkflowStep {
public:
    WorkflowGta5VehicleControlStep(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool toggleHeld_{false};
};

}  // namespace sdl3cpp::services::impl
