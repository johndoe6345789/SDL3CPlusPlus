#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.movers.init
 *
 * One-shot step (guard on q3.movers_initialized).
 * Reads bsp.entities and creates Q3Mover objects for each func_door / func_plat entity.
 *
 * Reads:  bsp.entities (nlohmann::json array)
 * Writes: q3.movers (MoverList), q3.movers_initialized (bool)
 */
class WorkflowQ3MoversInitStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MoversInitStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
