#include "services/interfaces/workflow/gta5/gta5_frame_stats_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <cstdio>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5FrameStatsStep::WorkflowGta5FrameStatsStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5FrameStatsStep::GetPluginId() const {
    return "gta5.frame.stats";
}

void WorkflowGta5FrameStatsStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_ || !logger_) return;
    const std::uint64_t now = SDL_GetTicksNS();
    if (last_ == 0) {
        windowStart_ = now;
    } else {
        worstMs_ = std::max(worstMs_, static_cast<double>(now - last_) / 1e6);
        ++frames_;
    }
    last_ = now;
    const double seconds = static_cast<double>(now - windowStart_) / 1e9;
    if (seconds < Gta5NumberOr(step, "interval_s", 2.f) || frames_ == 0) {
        return;
    }
    const auto eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    char line[280];
    std::snprintf(line, sizeof(line),
                  "gta5.frame: %.0f fps, worst %.1f ms, load %.2f ms, "
                  "cull %.2f ms, draw %.2f ms, %d draws, %zu groups, "
                  "%d texture binds, eye (%.0f, %.0f, %.0f)",
                  frames_ / seconds, worstMs_, state_->loadMs / frames_,
                  state_->cullMs / frames_, state_->drawMs / frames_,
                  context.Get<int>("gta5.tiles.drawn_last_frame", 0),
                  state_->batch.groups.size(), state_->textureBinds, eye.x,
                  eye.y, eye.z);
    logger_->Info(line);
    windowStart_ = now;
    frames_ = 0;
    worstMs_ = 0.0;
    state_->loadMs = 0.0;
    state_->cullMs = 0.0;
    state_->drawMs = 0.0;
}

}  // namespace sdl3cpp::services::impl
