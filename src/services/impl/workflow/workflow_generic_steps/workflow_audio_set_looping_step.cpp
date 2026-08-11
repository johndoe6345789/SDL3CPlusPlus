#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_set_looping_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowAudioSetLoopingStep::WorkflowAudioSetLoopingStep(std::shared_ptr<IAudioService> audioService,
                                                         std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioSetLoopingStep::GetPluginId() const {
    return "audio.set_looping";
}

void WorkflowAudioSetLoopingStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.set_looping requires an IAudioService");
    }

    WorkflowStepIoResolver resolver;
    const std::string loopKey = resolver.GetRequiredInputKey(step, "loop");
    const auto* loopValue = context.TryGet<bool>(loopKey);
    if (!loopValue) {
        throw std::runtime_error("audio.set_looping requires bool loop input");
    }

    try {
        audioService_->SetBackgroundLooping(*loopValue);

        if (logger_) {
            logger_->Trace("WorkflowAudioSetLoopingStep", "Execute",
                          "loop=" + std::string(*loopValue ? "true" : "false"),
                          "Updated background audio looping mode");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowAudioSetLoopingStep: Failed to set audio looping mode - " + std::string(e.what()));
        }
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
