#include "services/interfaces/workflow/workflow_media_item_select_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowMediaItemSelectStep::WorkflowMediaItemSelectStep(std::shared_ptr<IAudioService> audioService,
                                                         std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)),
      logger_(std::move(logger)) {}

std::string WorkflowMediaItemSelectStep::GetPluginId() const {
    return "media.item.select";
}

void WorkflowMediaItemSelectStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error("media.item.select requires an IAudioService for audio playback");
    }

    WorkflowStepIoResolver resolver;
    const std::string selectionKey = resolver.GetRequiredInputKey(step, "selection");
    const std::string statusKey = resolver.GetRequiredOutputKey(step, "status");

    // Get action parameter (optional, default "play")
    std::string action = "play";
    auto actionIt = step.parameters.find("action");
    if (actionIt != step.parameters.end()) {
        action = actionIt->second.stringValue;
    }

    const auto* selection = context.TryGet<MediaSelection>(selectionKey);
    if (!selection) {
        throw std::runtime_error("media.item.select missing selection input");
    }

    std::string status = "No selection";

    if (selection->hasSelection && selection->requestId != lastRequestId_) {
        lastRequestId_ = selection->requestId;
        const std::filesystem::path path = selection->path;

        if (path.empty()) {
            status = "Media path missing for selection";
            if (logger_) {
                logger_->Error("WorkflowMediaItemSelectStep::Execute: selection path missing");
            }
        } else if (!std::filesystem::exists(path)) {
            status = "Media file not found: " + path.string();
            if (logger_) {
                logger_->Error("WorkflowMediaItemSelectStep::Execute: media file not found " + path.string());
            }
        } else {
            // Handle action - currently supports audio playback
            if (action == "play") {
                try {
                    audioService_->PlayEffect(path, false);
                    status = "Playing \"" + selection->label + "\"";
                    if (logger_) {
                        logger_->Trace("WorkflowMediaItemSelectStep", "Execute",
                                       "item=" + selection->label + ", action=" + action,
                                       "Media playback dispatched");
                    }
                } catch (const std::exception& ex) {
                    status = "Failed to play \"" + selection->label + "\": " + ex.what();
                    if (logger_) {
                        logger_->Error("WorkflowMediaItemSelectStep::Execute: " + status);
                    }
                }
            } else {
                status = "Unknown action: " + action;
                if (logger_) {
                    logger_->Warn("WorkflowMediaItemSelectStep::Execute: unknown action '" + action + "'");
                }
            }
        }
    }

    context.Set(statusKey, status);
}

}  // namespace sdl3cpp::services::impl
