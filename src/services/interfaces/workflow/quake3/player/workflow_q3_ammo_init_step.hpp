#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.ammo.init
 *
 * One-shot step: writes q3.player_ammo with ioq3 default ammo counts.
 * Guards with q3.ammo_initialized — returns immediately if already done.
 *
 * Context writes:
 *   q3.player_ammo        nlohmann::json object of weapon->count
 *   q3.ammo_initialized   bool (true)
 */
class WorkflowQ3AmmoInitStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3AmmoInitStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
