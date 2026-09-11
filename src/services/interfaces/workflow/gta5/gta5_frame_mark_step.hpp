#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.frame.mark
 *
 * Notes how long the frame spent since the last mark, under the name in
 * its `phase` parameter. Placed between the frame's groups -- input,
 * physics, streaming, the swapchain acquire, render and submit -- it
 * says where a hitch went that no gta5 step timed itself;
 * gta5.frame.stats prints the marks with the hitch.
 */
class WorkflowGta5FrameMarkStep final : public IWorkflowStep {
public:
    WorkflowGta5FrameMarkStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl
