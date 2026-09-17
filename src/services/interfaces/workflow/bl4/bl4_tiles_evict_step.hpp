#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.tiles.evict
 *
 * Releases every tile on `state.pendingEvict`: each instance's physics
 * body, and its geometry's GPU buffers/collision shape once every tile
 * sharing that archetype has been released.
 *
 * Reads: gpu_device, physics_world
 */
class WorkflowBl4TilesEvictStep final : public IWorkflowStep {
public:
    WorkflowBl4TilesEvictStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
};

/**
 * Plugin ID: bl4.tiles.free
 *
 * Evicts every resident tile unconditionally. Run once before
 * system.exit, while the device and physics world still exist.
 */
class WorkflowBl4TilesFreeStep final : public IWorkflowStep {
public:
    WorkflowBl4TilesFreeStep(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl
