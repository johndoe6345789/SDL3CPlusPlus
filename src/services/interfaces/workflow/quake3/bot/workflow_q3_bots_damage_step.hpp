#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.damage.bots
 *
 * Applies pending bot damage from q3.bot_damage to each bot in q3.bots.
 * Sets bot["state"]="dead" when health drops to zero or below.
 * Clears q3.bot_damage after applying.
 *
 * Context reads:
 *   q3.bot_damage   nlohmann::json object  {"bot_0": int, ...}
 *   q3.bots         nlohmann::json array
 *
 * Context writes:
 *   q3.bots         nlohmann::json array (updated health / state)
 *   q3.bot_damage   nlohmann::json object (cleared to {})
 */
class WorkflowQ3BotsDamageStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3BotsDamageStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
