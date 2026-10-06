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

/// Headless dev runs stay silent unless SDL's dummy driver is chosen,
/// which plays to nowhere and so checks the audio path.
bool AudioAllowed() {
    const char* driver = std::getenv("SDL_AUDIO_DRIVER");
    const bool dummy = driver && std::string(driver) == "dummy";
    return !(EnvSet("SDL3CPP_HEADLESS") && !dummy) && !EnvSet("RACER_MUTE");
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
    // What should play: Anakin's theme on the title screens, the race's
    // music and pod sounds while racing, nothing when paused.
    const RacerPhase phase = state_->flow.phase;
    std::string wanted;
    if (phase == RacerPhase::Menu || phase == RacerPhase::Shop ||
        phase == RacerPhase::Loading) {
        wanted = "menu";
    } else if (state_->loaded && phase != RacerPhase::Paused) {
        wanted = "race " + std::to_string(state_->track.id);
    }
    if (wanted != playing_) {
        playing_ = wanted;
        mixer_.reset();
        if (!wanted.empty() && AudioAllowed()) {
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
    if (mixer_) mixer_->Update(*state_);
}

}  // namespace sdl3cpp::services::impl
