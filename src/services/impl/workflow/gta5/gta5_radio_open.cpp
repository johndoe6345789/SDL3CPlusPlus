#include "services/interfaces/workflow/gta5/gta5_radio_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_init.h>

#include <string>

namespace sdl3cpp::services::impl {

void WorkflowGta5RadioStep::Open(const WorkflowStepDefinition& step,
                                 WorkflowContext& context) {
    tried_ = true;
    volume_ = Gta5NumberOr(step, "volume", 0.5f);
    stations_ = ListGta5Stations(
        Gta5ParameterOr(step, "dir", ""),
        Gta5ResolvePath(step, context, "names",
                        "packages/gta5/data/radio_stations.json"));
    const bool standIn = stations_.empty();
    for (int v = 0; standIn && v < 3; ++v) {
        stations_.push_back({"Stand-in FM " + std::to_string(v + 1), {}, v});
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO) ||
        !(device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        nullptr))) {
        if (logger_) {
            logger_->Warn(std::string("gta5.radio: no audio: ") +
                          SDL_GetError());
        }
        return;
    }
    int frames = 0;
    if (!SDL_GetAudioDeviceFormat(device_, &spec_, &frames)) {
        spec_ = {SDL_AUDIO_F32, 2, 48000};
    }
    SDL_ResumeAudioDevice(device_);
    if (logger_) {
        logger_->Info("gta5.radio: " + std::to_string(stations_.size()) +
                      (standIn ? " stand-in" : "") + " stations");
    }
}

}  // namespace sdl3cpp::services::impl
