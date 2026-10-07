#include "services/interfaces/workflow/racer/audio/racer_audio_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"

namespace sdl3cpp::services::impl {

void WorkflowRacerAudioStep::Start(const WorkflowStepDefinition& step,
                                   const std::string& wanted) {
    playing_ = wanted;
    mixer_.reset();
    voice_.reset();
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
    // The player's racer talks during races.
    if (mixer_ && wanted != "menu") {
        voice_ = std::make_unique<RacerVoice>(
            RacerStringParam(step, "racer_dir", "RACER_DIR", ""),
            state_->racer.voice);
    }
}

}  // namespace sdl3cpp::services::impl
