#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_hud.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.hud
 *
 * The heads-up display over the finished frame: health and armour bars
 * (gta5.player.health, .armour, 0..100), the weapon in hand and its
 * ammunition (gta5.weapon.name, .clip, .reserve), and while driving a
 * speedometer and tachometer (gta5.car.speed, .revs, .gear). Hidden
 * under the Tab map. After the composite, before gta5.map.draw.
 */
class WorkflowGta5HudStep final : public IWorkflowStep {
public:
    WorkflowGta5HudStep(std::shared_ptr<ILogger> logger,
                        std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5Hud hud_;
};

}  // namespace sdl3cpp::services::impl
