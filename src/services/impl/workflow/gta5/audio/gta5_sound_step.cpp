#include "services/interfaces/workflow/gta5/audio/gta5_sound_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_playback.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_init.h>

#include <algorithm>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5SoundStep::WorkflowGta5SoundStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5SoundStep::GetPluginId() const {
    return "gta5.sound";
}

void WorkflowGta5SoundStep::Open(const WorkflowStepDefinition& step) {
    tried_ = true;
    volume_ = Gta5NumberOr(step, "volume", 1.f);
    water_ = LoadGta5WaterQuads(Gta5ParameterOr(step, "water_file", ""));
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO) ||
        !(device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        nullptr))) {
        if (logger_) {
            logger_->Warn(std::string("gta5.sound: no audio: ") +
                          SDL_GetError());
        }
        return;
    }
    int frames = 0;
    if (!SDL_GetAudioDeviceFormat(device_, &spec_, &frames)) {
        spec_ = {SDL_AUDIO_F32, 2, 48000};
    }
    const std::filesystem::path dir = Gta5ParameterOr(step, "dir", "");
    sounds_ = LoadGta5Sounds(dir, logger_);
    const std::string bank =
        Gta5ParameterOr(step, "engine_bank", "saloon_6_us_v8");
    if (LoadGta5EngineBank(dir / "engine" / bank, engineBank_) && logger_) {
        logger_->Info("gta5.sound: engine " + bank + ", granular");
    }
    SDL_ResumeAudioDevice(device_);
}

void WorkflowGta5SoundStep::Execute(const WorkflowStepDefinition& step,
                                    WorkflowContext& context) {
    if (!tried_) Open(step);
    if (!device_ || !state_) return;
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    ReapFinishedSoundStreams(playing_);
    Feet(context, dt);
    Engine(context, dt);
    Water(context);
    Shots(context);
}

}  // namespace sdl3cpp::services::impl
