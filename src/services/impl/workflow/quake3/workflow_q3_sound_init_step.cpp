#include "services/interfaces/workflow/quake3/workflow_q3_sound_init_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>

namespace sdl3cpp::services::impl {

WorkflowQ3SoundInitStep::WorkflowQ3SoundInitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3SoundInitStep::GetPluginId() const {
    return "q3.sound.init";
}

void WorkflowQ3SoundInitStep::Execute(const WorkflowStepDefinition&,
                                      WorkflowContext& context) {
    if (context.Contains("q3.sound.device")) {
        return;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        if (logger_) {
            logger_->Warn(std::string("q3.sound.init: no audio: ") +
                          SDL_GetError());
        }
        return;
    }

    const SDL_AudioDeviceID device =
        SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (device == 0) {
        if (logger_) {
            logger_->Warn(std::string("q3.sound.init: open failed: ") +
                          SDL_GetError());
        }
        return;
    }

    context.Set<SDL_AudioDeviceID>("q3.sound.device", device);
    if (logger_) {
        logger_->Info("q3.sound.init: audio device opened");
    }
}

}  // namespace sdl3cpp::services::impl
