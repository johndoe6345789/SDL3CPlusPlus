#include "services/interfaces/workflow/quake3/workflow_q3_sound_play_step.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_audio.h>

#include <algorithm>

namespace sdl3cpp::services::impl {

WorkflowQ3SoundPlayStep::WorkflowQ3SoundPlayStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3SoundPlayStep::GetPluginId() const {
    return "q3.sound.play";
}

void WorkflowQ3SoundPlayStep::ReapFinished() {
    // A sound that has been fully consumed is done; without this every
    // shot leaks a stream for the lifetime of the map.
    auto done = std::remove_if(
        playing_.begin(), playing_.end(), [](SDL_AudioStream* stream) {
            if (SDL_GetAudioStreamAvailable(stream) > 0) return false;
            SDL_UnbindAudioStream(stream);
            SDL_DestroyAudioStream(stream);
            return true;
        });
    playing_.erase(done, playing_.end());
}

void WorkflowQ3SoundPlayStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    ReapFinished();
    WorkflowStepParameterResolver params;

    // An optional gate keeps the trigger in the workflow: the step only
    // fires when the named bool is true this frame.
    const auto gate = step.inputs.find("when");
    if (gate != step.inputs.end() &&
        !context.GetBool(gate->second, false)) {
        return;
    }

    std::string name;
    if (const auto* p = params.FindParameter(step, "sound")) {
        name = p->stringValue;
    }
    const auto keyed = step.inputs.find("sound_key");
    if (keyed != step.inputs.end()) {
        name = context.Get<std::string>(keyed->second, name);
    }
    if (name.empty()) return;

    auto bank = context.Get<q3::SoundBankPtr>("q3.sound.bank", nullptr);
    const auto device =
        context.Get<SDL_AudioDeviceID>("q3.sound.device", 0);
    if (!bank || device == 0) return;

    const auto found = bank->find(name);
    if (found == bank->end() || found->second.pcm.empty()) return;

    // One stream per sound played: SDL mixes everything bound to the
    // device, so overlapping shots need no mixing of our own. The stream
    // frees itself once drained.
    const auto deviceSpec =
        context.Get<SDL_AudioSpec>("q3.sound.device_spec", SDL_AudioSpec{});
    SDL_AudioStream* stream =
        SDL_CreateAudioStream(&found->second.spec, &deviceSpec);
    if (!stream) {
        if (logger_) {
            logger_->Warn(std::string("q3.sound.play: stream: ") +
                          SDL_GetError());
        }
        return;
    }
    SDL_PutAudioStreamData(stream, found->second.pcm.data(),
                           static_cast<int>(found->second.pcm.size()));
    SDL_FlushAudioStream(stream);
    if (!SDL_BindAudioStream(device, stream)) {
        if (logger_) {
            logger_->Warn(std::string("q3.sound.play: bind: ") +
                          SDL_GetError());
        }
        SDL_DestroyAudioStream(stream);
        return;
    }
    SDL_ResumeAudioDevice(device);
    playing_.push_back(stream);
}

}  // namespace sdl3cpp::services::impl
