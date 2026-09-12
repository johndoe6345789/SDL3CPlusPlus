#include "services/interfaces/workflow/gta5/stream/gta5_frame_stats_step.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_hitch_report.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
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
        const double ms = static_cast<double>(now - last_) / 1e6;
        worstMs_ = std::max(worstMs_, ms);
        ++frames_;
        if (ms > Gta5NumberOr(step, "hitch_ms", 12.f) &&
            context.GetString("gta5.loading.text", "").empty()) {
            ReportGta5Hitch(logger_, state_->cost, ms);
        }
    }
    last_ = now;
    window_.Add(state_->cost);
    state_->cost = {};
    state_->cost.lastMark = now;  // the next frame's marks count from here
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
                  frames_ / seconds, worstMs_, window_.load / frames_,
                  window_.cull / frames_, window_.draw / frames_,
                  context.Get<int>("gta5.tiles.drawn_last_frame", 0),
                  state_->batch.groups.size(), state_->textureBinds, eye.x,
                  eye.y, eye.z);
    logger_->Info(line);
    windowStart_ = now;
    frames_ = 0;
    worstMs_ = 0.0;
    window_ = {};
}

}  // namespace sdl3cpp::services::impl
