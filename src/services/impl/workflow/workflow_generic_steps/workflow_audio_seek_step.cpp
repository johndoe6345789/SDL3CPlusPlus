#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_seek_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowAudioSeekStep::WorkflowAudioSeekStep(std::shared_ptr<IAudioService> audioService,
                                             std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioSeekStep::GetPluginId() const {
    return "audio.seek";
}

void WorkflowAudioSeekStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.seek requires an IAudioService");
    }

    WorkflowStepIoResolver resolver;
    const std::string positionKey = resolver.GetRequiredInputKey(step, "position");
    const auto* positionValue = context.TryGet<double>(positionKey);
    if (!positionValue) {
        throw std::runtime_error("audio.seek requires numeric position input (milliseconds)");
    }

    if (*positionValue < 0.0) {
        throw std::runtime_error("audio.seek position must be non-negative");
    }

    const uint32_t positionMs = static_cast<uint32_t>(*positionValue);

    try {
        audioService_->SeekBackground(positionMs);

        if (logger_) {
            logger_->Trace("WorkflowAudioSeekStep", "Execute",
                          "position_ms=" + std::to_string(positionMs),
                          "Seeked background audio to position");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowAudioSeekStep: Failed to seek audio - " + std::string(e.what()));
        }
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
