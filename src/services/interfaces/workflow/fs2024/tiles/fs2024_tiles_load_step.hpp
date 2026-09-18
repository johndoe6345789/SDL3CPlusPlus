#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_finish_ready.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.tiles.load
 *
 * Streams tiles in the background: queues `state.pendingLoad` on the
 * loader threads (starting them with the world), drops queued tiles the
 * viewer has moved on from, and finishes built ones -- GPU uploads and
 * collision -- within the frame's `finish_budget_ms`. A tile whose
 * build failed is remembered as missing rather than retried every
 * call. Logs how streaming is keeping up every five seconds.
 *
 * Parameters: force (bool, default false) -- wait for every queued
 *             tile; used once at init so the spawn point's ground
 *             exists before the first frame.
 * Reads: gpu_device, physics_world
 */
class WorkflowFs2024TilesLoadStep final : public IWorkflowStep {
public:
    WorkflowFs2024TilesLoadStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void ReportStats();

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
    Fs2024FinishStats stats_;
    std::chrono::steady_clock::time_point lastReport_{};
};

}  // namespace sdl3cpp::services::impl
