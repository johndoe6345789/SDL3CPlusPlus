#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/effects/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.effects.draw
 *
 * Muzzle flashes, tracers, the dust off a hit, fireballs, smoke and the
 * scorch left behind (gta5.effects): quads built in world space each
 * frame -- facing the camera, along a tracer's line, or lying on what
 * they mark -- and drawn over the scene, blended, after the water. Their
 * sprites are drawn at load, not loaded. Carries them forward by
 * physics_dt first, so they move whether anything is firing or not.
 */
class WorkflowGta5EffectsDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5EffectsDrawStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl
