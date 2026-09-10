#include "services/interfaces/workflow/quake3/workflow_q3_sound_play_step.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_playback.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_audio.h>

namespace sdl3cpp::services::impl {

WorkflowQ3SoundPlayStep::WorkflowQ3SoundPlayStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3SoundPlayStep::GetPluginId() const {
    return "q3.sound.play";
}

void WorkflowQ3SoundPlayStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    ReapFinishedSoundStreams(playing_);
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

    const auto deviceSpec =
        context.Get<SDL_AudioSpec>("q3.sound.device_spec", SDL_AudioSpec{});
    SDL_AudioStream* stream =
        PlaySoundOnDevice(found->second, device, deviceSpec, logger_);
    if (stream) {
        playing_.push_back(stream);
    }
}

}  // namespace sdl3cpp::services::impl
