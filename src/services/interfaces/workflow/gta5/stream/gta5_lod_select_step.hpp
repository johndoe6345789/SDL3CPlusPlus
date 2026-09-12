#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.lod.select
 *
 * Re-evaluates each resident tile against the LOD rings and flags any
 * whose band changed, so evict tears it down and load rebuilds it at the
 * new detail level.
 *
 * Banding is per tile rather than per placement. That matches how GTA V
 * behaves at range, where whole blocks drop to a merged SLOD mesh, and it
 * keeps a band change to one teardown instead of thousands.
 *
 * Writes: gta5.lod.rebuild_count
 */
class WorkflowGta5LodSelectStep final : public IWorkflowStep {
public:
    WorkflowGta5LodSelectStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl
