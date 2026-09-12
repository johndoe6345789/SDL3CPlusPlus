#include "services/interfaces/workflow/gta5/audio/gta5_radio_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>

#include <chrono>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5RadioStep::WorkflowGta5RadioStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5RadioStep::GetPluginId() const {
    return "gta5.radio";
}

void WorkflowGta5RadioStep::Execute(const WorkflowStepDefinition& step,
                                    WorkflowContext& context) {
    if (!tried_) Open(step, context);
    if (!device_ || !state_ || stations_.empty()) return;
    const bool seated = state_->seated >= 0;
    if (seated && !seated_) {
        station_ = std::uniform_int_distribution<int>(
            0, static_cast<int>(stations_.size()) - 1)(rng_);
        midway_ = true;
        shownUntilMs_ = SDL_GetTicks() + 4000;
        if (logger_) logger_->Info("gta5.radio: " + stations_[station_].name);
        Tune();
    } else if (!seated && seated_) {
        Stop();
    }
    seated_ = seated;
    const bool ready =
        loading_.valid() && loading_.wait_for(std::chrono::seconds(0)) ==
                                std::future_status::ready;
    if (seated && ready) Play(loading_.get());
    // The track ran out: the next from the same station.
    if (seated && !loading_.valid() &&
        (!stream_ || SDL_GetAudioStreamQueued(stream_) == 0)) {
        Tune();
    }
    const bool shown = seated && SDL_GetTicks() < shownUntilMs_;
    context.Set<std::string>("gta5.radio.text",
                             shown ? stations_[station_].name : "");
}

}  // namespace sdl3cpp::services::impl
