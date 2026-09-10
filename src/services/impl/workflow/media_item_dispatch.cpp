#include "services/interfaces/workflow/media_item_dispatch.hpp"

#include <filesystem>

namespace sdl3cpp::services::impl {

std::string DispatchMediaSelection(IAudioService& audioService, ILogger* logger,
                                   const std::string& action,
                                   const MediaSelection& selection) {
    const std::filesystem::path path = selection.path;

    if (path.empty()) {
        if (logger) {
            logger->Error(
                "WorkflowMediaItemSelectStep::Execute: selection path "
                "missing");
        }
        return "Media path missing for selection";
    }

    if (!std::filesystem::exists(path)) {
        if (logger) {
            logger->Error(
                "WorkflowMediaItemSelectStep::Execute: media file not "
                "found " +
                path.string());
        }
        return "Media file not found: " + path.string();
    }

    if (action != "play") {
        if (logger) {
            logger->Warn(
                "WorkflowMediaItemSelectStep::Execute: unknown action '" +
                action + "'");
        }
        return "Unknown action: " + action;
    }

    try {
        audioService.PlayEffect(path, false);
        if (logger) {
            logger->Trace("WorkflowMediaItemSelectStep", "Execute",
                          "item=" + selection.label + ", action=" + action,
                          "Media playback dispatched");
        }
        return "Playing \"" + selection.label + "\"";
    } catch (const std::exception& ex) {
        const std::string status =
            "Failed to play \"" + selection.label + "\": " + ex.what();
        if (logger) {
            logger->Error("WorkflowMediaItemSelectStep::Execute: " + status);
        }
        return status;
    }
}

}  // namespace sdl3cpp::services::impl
