#include "services/interfaces/workflow/quake3/workflow_q3_frame_time_step.hpp"
#include "services/interfaces/workflow/quake3/q3_frame_time.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>

namespace sdl3cpp::services::impl {

WorkflowQ3FrameTimeStep::WorkflowQ3FrameTimeStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3FrameTimeStep::GetPluginId() const {
    return "time.frame_delta";
}

void WorkflowQ3FrameTimeStep::Execute(const WorkflowStepDefinition&,
                                      WorkflowContext& context) {
    const uint64_t now = SDL_GetTicksNS();
    if (lastTicksNs_ == 0) {
        lastTicksNs_ = now;  // first frame has no previous to measure from
    }

    const double seconds =
        static_cast<double>(now - lastTicksNs_) / 1'000'000'000.0;
    lastTicksNs_ = now;

    const float delta =
        q3::ClampFrameSeconds(static_cast<float>(seconds));
    elapsedSeconds_ += delta;

    context.Set<double>("frame.delta_time", static_cast<double>(delta));
    context.Set<double>("frame.elapsed_time", elapsedSeconds_);
}

}  // namespace sdl3cpp::services::impl
