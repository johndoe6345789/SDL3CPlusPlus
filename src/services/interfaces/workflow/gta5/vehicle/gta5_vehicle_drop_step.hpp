#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.vehicle.drop
 *
 * Drops one GTA V vehicle onto whatever ground the physics world holds
 * under it, once. Unlike gta5.vehicle.spawn it does not wait for the map
 * to stream in, so any package with its own ground can use it.
 *
 * Reads:  gpu_device, physics_world
 * Writes: nothing; the vehicle joins the state vehicle list
 */
class WorkflowGta5VehicleDropStep final : public IWorkflowStep {
public:
    WorkflowGta5VehicleDropStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool attempted_{false};
};

}  // namespace sdl3cpp::services::impl
