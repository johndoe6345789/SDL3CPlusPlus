#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: time.frame_delta
 *
 * Publishes the real time elapsed since the previous frame as
 * frame.delta_time. Eight movement and physics steps read that key and
 * nothing wrote it, so they all used their own hardcoded default and
 * the player moved a fixed distance per frame rather than per second.
 *
 * Writes: frame.delta_time (seconds), frame.elapsed_time (seconds)
 */
class WorkflowQ3FrameTimeStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3FrameTimeStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    uint64_t lastTicksNs_{0};
    double elapsedSeconds_{0.0};
};

}  // namespace sdl3cpp::services::impl
