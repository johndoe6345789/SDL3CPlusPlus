#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Turns this frame's axes and buttons into navigation edges: a stick or
/// key counts once when pushed past half way, not every frame it is held.
class RacerNavReader {
public:
    RacerNav Read(const WorkflowContext& context);

private:
    bool up_ = false, down_ = false, left_ = false, right_ = false;
    bool select_ = false, back_ = false;
};

/**
 * Plugin ID: racer.flow
 *
 * Runs the menus, pod shop, pause and results from input.move_forward,
 * input.move_right, racer.select_pressed and racer.back_pressed, and
 * publishes racer.quit_requested when the player quits.
 */
class WorkflowRacerFlowStep final : public IWorkflowStep {
public:
    WorkflowRacerFlowStep(std::shared_ptr<ILogger> logger,
                          std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    RacerNavReader nav_;
};

}  // namespace sdl3cpp::services::impl
