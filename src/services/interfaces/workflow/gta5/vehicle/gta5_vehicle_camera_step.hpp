#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.vehicle.camera
 *
 * While seated, replaces the first-person view with a chase camera
 * behind and above the car. Seated, the player is pinned 0.8 m above the
 * chassis centre, so the first-person view is from inside the roof.
 *
 * Runs after gta5.vehicles.sync and before render.prepare, so it
 * overrides what camera.fps.update wrote this frame. Keeps the
 * projection; replaces view and position.
 *
 * Reads/writes: camera.state
 */
class WorkflowGta5VehicleCameraStep final : public IWorkflowStep {
public:
    WorkflowGta5VehicleCameraStep(std::shared_ptr<ILogger> logger,
                                  std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    /// Eased so suspension bounce and kerb strikes do not shake the view.
    glm::vec3 eye_{0.f};
    bool placed_{false};
};

}  // namespace sdl3cpp::services::impl
