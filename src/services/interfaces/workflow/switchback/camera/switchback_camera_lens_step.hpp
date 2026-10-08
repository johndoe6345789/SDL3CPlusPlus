#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.camera.lens
 *
 * Rebuilds the projection in camera.state with the clip range the race needs.
 * camera.setup only offers a 100 m far plane, which cuts the track short.
 *
 * Parameters: fov (default 60, degrees),
 *             near (default 0.5, metres),
 *             far (default 4000, metres)
 * Reads/writes: camera.state
 */
class WorkflowSwitchbackCameraLensStep final : public IWorkflowStep {
public:
    explicit WorkflowSwitchbackCameraLensStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
