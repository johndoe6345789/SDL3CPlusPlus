#include "workflow_soundboard_audio_step.hpp"

#include "workflow_step_io_resolver.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSoundboardAudioStep::WorkflowSoundboardAudioStep(std::shared_ptr<IAudioService> audioService,
                                                         std::shared_ptr<ISoundboardStateService> stateService,
                                                         std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      stateService_(std::move(stateService)),
      logger_(std::move(logger)) {}

std::string WorkflowSoundboardAudioStep::GetPluginId() const {
    return "soundboard.audio";
}

void WorkflowSoundboardAudioStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("soundboard.audio requires an IAudioService");
    }

    WorkflowStepIoResolver resolver;
    const std::string selectionKey = resolver.GetRequiredInputKey(step, "selection");
    const std::string statusKey = resolver.GetRequiredOutputKey(step, "status");

    const auto* selection = context.TryGet<SoundboardSelection>(selectionKey);
    if (!selection) {
        throw std::runtime_error("soundboard.audio missing selection input");
    }

    std::string status = stateService_
        ? stateService_->GetStatusMessage()
        : std::string("Select a clip to play");

    if (selection->hasSelection && selection->requestId != lastRequestId_) {
        lastRequestId_ = selection->requestId;
        const std::filesystem::path path = selection->path;
        if (path.empty()) {
            status = "Audio path missing for selection";
            if (logger_) {
                logger_->Error("WorkflowSoundboardAudioStep::Execute: selection path missing");
            }
        } else if (!std::filesystem::exists(path)) {
            status = "Audio file not found: " + path.string();
            if (logger_) {
                logger_->Error("WorkflowSoundboardAudioStep::Execute: audio file not found " + path.string());
            }
        } else {
            try {
                audioService_->PlayEffect(path, false);
                status = "Playing \"" + selection->label + "\"";
                if (logger_) {
                    logger_->Trace("WorkflowSoundboardAudioStep", "Execute",
                                   "clip=" + selection->label,
                                   "Audio playback dispatched");
                }
            } catch (const std::exception& ex) {
                status = "Failed to play \"" + selection->label + "\": " + ex.what();
                if (logger_) {
                    logger_->Error("WorkflowSoundboardAudioStep::Execute: " + status);
                }
            }
        }

        if (stateService_) {
            stateService_->SetStatusMessage(status);
        }
    }

    context.Set(statusKey, status);
}

}  // namespace sdl3cpp::services::impl
