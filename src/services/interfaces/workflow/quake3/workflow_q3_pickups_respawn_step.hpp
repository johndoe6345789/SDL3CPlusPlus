#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pickups.respawn
 *
 * Re-enables collected pickups once their respawn timer has elapsed.
 * Sets collected[entityId]=false when elapsed > pickup_respawn_times[entityId].
 *
 * Context reads:
 *   q3.collected            nlohmann::json object  {entityId: bool}
 *   q3.pickup_respawn_times nlohmann::json object  {entityId: double}
 *   frame.elapsed           double
 *
 * Context writes:
 *   q3.collected            nlohmann::json object  (respawned items cleared)
 */
class WorkflowQ3PickupsRespawnStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PickupsRespawnStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
