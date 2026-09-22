#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.camera.chase
 *
 * Follows the car from behind and a little above in Chase mode,
 * easing toward its heading rather than snapping to it so a corner
 * reads as a corner; sits at the driver's eye position facing
 * forward in Cockpit mode, the original's other main view. `stunts.
 * camera_toggle_pressed` (bound to C by default) switches between
 * them on its rising edge. Publishes `camera.state` for
 * render.prepare, and `stunts.camera_mode` ("chase"/"cockpit") for
 * stunts.car.draw to hide the body from inside it.
 *
 * Parameters: `distance`, `height`, `lead`, `fov`, `near`, `far`,
 * `follow` (0..1, how quickly Chase takes up a new heading), and
 * `eye_height`/`eye_forward` for where Cockpit sits in the cabin.
 */
class WorkflowStuntsChaseCameraStep final : public IWorkflowStep {
public:
    explicit WorkflowStuntsChaseCameraStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    float heading_ = 0.f;
    bool started_ = false;
    bool cockpit_ = false;
    bool toggleWasDown_ = false;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
