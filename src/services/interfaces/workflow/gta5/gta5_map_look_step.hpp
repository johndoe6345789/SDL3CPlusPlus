#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.map.look
 *
 * While the Tab map is open (gta5.map.open, as the map left it last
 * frame) the mouse is the map's: gta5.mouse_look -- movement_active
 * with no map -- goes false, so input.mouse.grab frees the cursor, and
 * this frame's mouse motion is zeroed, so the view holds still. Runs in
 * the input group, before input.mouse.grab and the camera.
 */
class WorkflowGta5MapLookStep final : public IWorkflowStep {
public:
    explicit WorkflowGta5MapLookStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
