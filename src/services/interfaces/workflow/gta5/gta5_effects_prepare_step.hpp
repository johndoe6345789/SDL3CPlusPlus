#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.effects.prepare
 *
 * Carries the effects forward and stages this frame quads through the
 * upload batch, which gta5.tiles.cull submits. It runs before that, and
 * before the render pass: a copy cannot be made while a pass is open,
 * which is why the drawing itself only binds and draws.
 */
class WorkflowGta5EffectsPrepareStep final : public IWorkflowStep {
public:
    WorkflowGta5EffectsPrepareStep(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool tried_{false};
};

}  // namespace sdl3cpp::services::impl
