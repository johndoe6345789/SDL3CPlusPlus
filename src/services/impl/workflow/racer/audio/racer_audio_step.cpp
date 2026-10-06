#include "services/interfaces/workflow/racer/audio/racer_audio_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#include <cstdlib>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

bool EnvSet(const char* name) {
    const char* value = std::getenv(name);
    return value && value[0] != 0 && std::string(value) != "0";
}

}  // namespace

WorkflowRacerAudioStep::WorkflowRacerAudioStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerAudioStep::GetPluginId() const {
    return "racer.audio.update";
}

void WorkflowRacerAudioStep::Execute(const WorkflowStepDefinition& step,
                                     WorkflowContext&) {
    if (!state_->loaded) return;
    if (!tried_) {
        tried_ = true;
        // Headless dev runs stay silent unless SDL's dummy driver is
        // chosen, which plays to nowhere and so checks the audio path.
        const char* driver = std::getenv("SDL_AUDIO_DRIVER");
        const bool dummy = driver && std::string(driver) == "dummy";
        if ((EnvSet("SDL3CPP_HEADLESS") && !dummy) || EnvSet("RACER_MUTE")) {
            return;
        }
        mixer_ = std::make_unique<RacerAudioMixer>();
        const std::string dir =
            RacerStringParam(step, "racer_dir", "RACER_DIR", "");
        const bool open = mixer_->Open(dir, state_->track.planet);
        if (logger_) {
            logger_->Info("racer.audio.update: " +
                          (open ? std::to_string(mixer_->LoadedClips()) +
                                      " of 6 sounds loaded"
                                : std::string("no audio device")));
        }
    }
    if (mixer_) mixer_->Update(*state_);
}

}  // namespace sdl3cpp::services::impl
