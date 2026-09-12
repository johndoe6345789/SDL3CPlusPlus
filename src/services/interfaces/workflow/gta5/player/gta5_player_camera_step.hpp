#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.player.camera
 *
 * Third person on foot, on from the start; V swaps to first person and
 * back. The camera sits `distance` behind the head along the view and
 * `height` above it, orbiting with the mouse, and is pulled in front of
 * any wall between it and the head. Runs after camera.fps.update, whose
 * view it replaces, and publishes gta5.third_person for the character.
 * In the car it does nothing: gta5.vehicle.camera has the view.
 */
class WorkflowGta5PlayerCameraStep final : public IWorkflowStep {
public:
    WorkflowGta5PlayerCameraStep(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool third_{true};
    bool held_{false};
};

}  // namespace sdl3cpp::services::impl
