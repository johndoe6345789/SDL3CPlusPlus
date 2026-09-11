#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.frame.stats
 *
 * Every interval_s (default 2), logs the frame rate, the worst frame,
 * and the CPU time gta5.tiles.load, cull and draw took on average:
 * enough to see where a frame goes without --trace, whose own logging
 * costs more than the frame. Once the player is free to move, any frame
 * over hitch_ms (default 12) is broken down step by step as it happens.
 * Runs last in the frame.
 */
class WorkflowGta5FrameStatsStep final : public IWorkflowStep {
public:
    WorkflowGta5FrameStatsStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::uint64_t windowStart_{0};
    std::uint64_t last_{0};
    int frames_{0};
    double worstMs_{0.0};
    Gta5FrameCost window_;
};

}  // namespace sdl3cpp::services::impl
