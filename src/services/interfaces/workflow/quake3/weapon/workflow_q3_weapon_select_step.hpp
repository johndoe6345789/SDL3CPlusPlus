#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.weapon.select
 *
 * Scans number-key inputs 1-9. If the key is pressed and the requested weapon
 * is present in q3.inventory, updates q3.current_weapon.
 *
 * Reads:  input_key_1 … input_key_9 (bool)
 *         q3.inventory (nlohmann::json)
 *         q3.current_weapon (string)
 * Writes: q3.current_weapon (string)
 *         q3.inventory (string — ensures gauntlet + machinegun always present)
 */
class WorkflowQ3WeaponSelectStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3WeaponSelectStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
