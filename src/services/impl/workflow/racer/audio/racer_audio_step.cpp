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
                                     WorkflowContext&) {
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
    if (wanted != playing_) {
        playing_ = wanted;
        mixer_.reset();
        if (!wanted.empty() && RacerAudioAllowed()) {
            mixer_ = std::make_unique<RacerAudioMixer>();
            const std::string dir =
                RacerStringParam(step, "racer_dir", "RACER_DIR", "");
            const bool menu = wanted == "menu";
            const bool open = mixer_->Open(
                dir, menu ? "Menu" : state_->track.planet, menu);
            if (logger_) {
                logger_->Info("racer.audio.update: " + wanted + ", " +
                              std::to_string(mixer_->LoadedClips()) +
                              (open ? " sounds" : " (no audio device)"));
            }
        }
    }
    if (!mixer_) return;
    mixer_->SetPaused(phase == RacerPhase::Paused);
    mixer_->Update(*state_);
}

}  // namespace sdl3cpp::services::impl
