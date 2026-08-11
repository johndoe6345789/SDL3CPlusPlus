#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_set_volume_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowAudioSetVolumeStep::WorkflowAudioSetVolumeStep(std::shared_ptr<IAudioService> audioService,
                                                       std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioSetVolumeStep::GetPluginId() const {
    return "audio.set_volume";
}

void WorkflowAudioSetVolumeStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.set_volume requires an IAudioService");
    }

    WorkflowStepIoResolver resolver;
    const std::string valueKey = resolver.GetRequiredInputKey(step, "value");
    const auto* value = context.TryGet<double>(valueKey);
    if (!value) {
        throw std::runtime_error("audio.set_volume requires numeric value input");
    }

    const float clamped = static_cast<float>(std::clamp(*value, 0.0, 1.0));
    audioService_->SetVolume(clamped);

    if (logger_) {
        logger_->Trace("WorkflowAudioSetVolumeStep", "Execute",
                       "value=" + std::to_string(clamped),
                       "Updated audio volume");
    }
}

}  // namespace sdl3cpp::services::impl
