#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/quake3/q3_sky_resources.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws the map's sky as a camera-locked cloud dome.
 *
 * Plugin ID: "q3.sky.draw"
 *
 * bsp.build_polygons drops sky brush faces (they are a portal to the
 * sky, not a surface), so without this the opening shows the frame's
 * clear colour. Draw it before the world and inside the same render
 * pass: the dome sits at a fixed radius around the camera, so world
 * geometry is always nearer and depth-tests over it.
 */
class WorkflowQ3SkyDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3SkyDrawStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    SkyResources resources_;
};

}  // namespace sdl3cpp::services::impl
