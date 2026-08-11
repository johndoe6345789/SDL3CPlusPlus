#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_pause_step.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowAudioPauseStep::WorkflowAudioPauseStep(std::shared_ptr<IAudioService> audioService,
                                               std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioPauseStep::GetPluginId() const {
    return "audio.pause";
}

void WorkflowAudioPauseStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.pause requires an IAudioService");
    }

    try {
        audioService_->PauseBackground();

        if (logger_) {
            logger_->Trace("WorkflowAudioPauseStep", "Execute",
                          "",
                          "Paused background audio playback");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowAudioPauseStep: Failed to pause audio - " + std::string(e.what()));
        }
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
