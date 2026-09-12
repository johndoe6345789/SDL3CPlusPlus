#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.tiles.evict
 *
 * Drops every scene object belonging to a tile outside the evict radius,
 * and to any tile flagged for rebuild by gta5.lod.select.
 *
 * Objects are matched on the "gta5:<x>_<z>" tag that gta5.tiles.load
 * writes into SceneObject::objectType, as model.despawn matches.
 *
 * Writes: the scene object list named by the objects_key parameter
 */
class WorkflowGta5TilesEvictStep final : public IWorkflowStep {
public:
    WorkflowGta5TilesEvictStep(std::shared_ptr<ILogger> logger,
                                  std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl
