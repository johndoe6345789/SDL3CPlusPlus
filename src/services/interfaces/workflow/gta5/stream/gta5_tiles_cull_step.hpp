#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.tiles.cull
 *
 * Culls resident instances to the view, groups the visible ones by
 * archetype and uploads their model matrices for gta5.tiles.draw to draw
 * instanced. Runs after render.prepare, which publishes the camera, and
 * before the frame's render pass begins: an upload cannot happen inside
 * one. Parameter cull_size_ratio (default 0.003) also drops instances
 * smaller than that fraction of their distance.
 */
class WorkflowGta5TilesCullStep final : public IWorkflowStep {
public:
    WorkflowGta5TilesCullStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    bool warned_{false};
    bool collisionView_{false};  // F2
    bool pickHeld_{false};
    bool toggleHeld_{false};
};

}  // namespace sdl3cpp::services::impl
