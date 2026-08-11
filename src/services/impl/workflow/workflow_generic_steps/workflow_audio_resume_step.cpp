#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_resume_step.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowAudioResumeStep::WorkflowAudioResumeStep(std::shared_ptr<IAudioService> audioService,
                                                 std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowAudioResumeStep::GetPluginId() const {
    return "audio.resume";
}

void WorkflowAudioResumeStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("audio.resume requires an IAudioService");
    }

    try {
        audioService_->ResumeBackground();

        if (logger_) {
            logger_->Trace("WorkflowAudioResumeStep", "Execute",
                          "",
                          "Resumed background audio playback");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowAudioResumeStep: Failed to resume audio - " + std::string(e.what()));
        }
        throw;
    }
}

}  // namespace sdl3cpp::services::impl
