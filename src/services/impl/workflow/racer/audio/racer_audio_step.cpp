#include "services/interfaces/workflow/racer/audio/racer_audio_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#include <cstdlib>
#include <utility>

namespace sdl3cpp::services::impl {
WorkflowRacerAudioStep::WorkflowRacerAudioStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerAudioStep::GetPluginId() const {
    return "racer.audio.update";
}

void WorkflowRacerAudioStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext& context) {
    // What should play: Anakin's theme on the title screens, the race's
    // music and pod sounds while racing (frozen while paused).
    const RacerPhase phase = state_->flow.phase;
    std::string wanted;
    const bool inRace = phase == RacerPhase::Racing ||
                        phase == RacerPhase::Paused ||
                        phase == RacerPhase::Results;
    if (phase == RacerPhase::Cutscene) {
        wanted = "";  // the cutscene plays its own sound
    } else if (!inRace) {
        wanted = "menu";
    } else if (state_->loaded) {
        wanted = "race " + std::to_string(state_->track.id);
    }
    if (wanted != playing_) Start(step, wanted);
    const float dt =
        static_cast<float>(context.Get<double>("frame.delta_time", 0.0));
    if (mixer_) {
        mixer_->SetPaused(phase == RacerPhase::Paused);
        mixer_->Update(*state_);
    }
    if (voice_) {
        voice_->SetPaused(phase == RacerPhase::Paused);
        if (phase != RacerPhase::Paused) voice_->Update(*state_, dt);
    }
    state_->hazardSounds = 0;  // heard, or dropped when silent
}

}  // namespace sdl3cpp::services::impl
