#include "services/interfaces/workflow/gta5/stream/gta5_frame_mark_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"

#include <SDL3/SDL_timer.h>

#include <cstdio>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5FrameMarkStep::WorkflowGta5FrameMarkStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5FrameMarkStep::GetPluginId() const {
    return "gta5.frame.mark";
}

void WorkflowGta5FrameMarkStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& /*context*/) {
    if (!state_) return;
    Gta5FrameCost& cost = state_->cost;
    const std::uint64_t now = SDL_GetTicksNS();
    if (cost.lastMark != 0) {
        char part[64];
        std::snprintf(part, sizeof(part), "%s%s %.1f",
                      cost.phases.empty() ? "" : ", ",
                      Gta5ParameterOr(step, "phase", "?").c_str(),
                      static_cast<double>(now - cost.lastMark) / 1e6);
        cost.phases += part;
    }
    cost.lastMark = now;
}

}  // namespace sdl3cpp::services::impl
