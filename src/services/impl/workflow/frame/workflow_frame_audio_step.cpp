#include "workflow_frame_audio_step.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameAudioStep::WorkflowFrameAudioStep(std::shared_ptr<IAudioService> audioService,
                                               std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowFrameAudioStep::GetPluginId() const {
    return "frame.audio";
}

void WorkflowFrameAudioStep::Execute(const WorkflowStepDefinition&, WorkflowContext&) {
    if (!audioService_) {
        throw std::runtime_error("frame.audio requires an IAudioService");
    }
    audioService_->Update();
    if (logger_) {
        logger_->Trace("WorkflowFrameAudioStep", "Execute", "", "Audio updated");
    }
}

}  // namespace sdl3cpp::services::impl
