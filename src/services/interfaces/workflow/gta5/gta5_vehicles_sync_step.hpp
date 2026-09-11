#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.vehicles.sync
 *
 * Copies each vehicle body transform into the matrix it draws with, so
 * what Bullet does to a car is what you see.
 */
class WorkflowGta5VehiclesSyncStep final : public IWorkflowStep {
public:
    WorkflowGta5VehiclesSyncStep(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool spawned_{false};
};

}  // namespace sdl3cpp::services::impl
