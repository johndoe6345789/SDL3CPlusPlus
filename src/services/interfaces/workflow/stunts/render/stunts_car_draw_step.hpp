#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.car.draw
 *
 * Draws the car's own body: its flat-shaded panels (real geometry,
 * coloured from the palette texture `texture_key` names, default
 * "stunts_palette") and its wheels (discs built from the body's own
 * Wheel primitives), placed and turned by `stunts.car_pos` and
 * `stunts.car_heading`.
 */
class WorkflowStuntsCarDrawStep final : public IWorkflowStep {
public:
    WorkflowStuntsCarDrawStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<StuntsWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool traced_ = false;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<StuntsWorldState> state_;
};

}  // namespace sdl3cpp::services::impl
