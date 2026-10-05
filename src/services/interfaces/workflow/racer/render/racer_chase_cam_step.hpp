#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.camera.chase
 *
 * The game's default view: low behind the pod, easing after its heading
 * and pulling back as speed rises. Writes camera.state for render.prepare
 * and render.camera_pos. Parameters: distance, height, lead, fov, near,
 * far, aspect.
 */
class WorkflowRacerChaseCameraStep final : public IWorkflowStep {
public:
    WorkflowRacerChaseCameraStep(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    float heading_ = 0.f;
    glm::vec3 eye_{0.f};
    bool started_ = false;
};

}  // namespace sdl3cpp::services::impl
