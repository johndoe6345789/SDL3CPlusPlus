#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.tiles.evict
 *
 * Releases every tile on `state.pendingEvict`: its GPU texture, mesh
 * buffers and collision body, then drops it from `state.resident`.
 *
 * Reads: gpu_device, physics_world
 */
class WorkflowFs2024TilesEvictStep final : public IWorkflowStep {
public:
    WorkflowFs2024TilesEvictStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
};

/**
 * Plugin ID: fs2024.tiles.free
 *
 * Evicts every resident tile unconditionally. Run once before
 * system.exit, while the device and physics world still exist.
 */
class WorkflowFs2024TilesFreeStep final : public IWorkflowStep {
public:
    WorkflowFs2024TilesFreeStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl
