#include "services/interfaces/workflow/workflow_media_item_select_step.hpp"

#include "services/interfaces/workflow/media_item_dispatch.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowMediaItemSelectStep::WorkflowMediaItemSelectStep(
    std::shared_ptr<IAudioService> audioService,
    std::shared_ptr<ILogger> logger)
    : audioService_(std::move(audioService)), logger_(std::move(logger)) {}

std::string WorkflowMediaItemSelectStep::GetPluginId() const {
    return "media.item.select";
}

void WorkflowMediaItemSelectStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (!audioService_) {
        throw std::runtime_error(
            "media.item.select requires an IAudioService for audio "
            "playback");
    }

    WorkflowStepIoResolver resolver;
    const std::string selectionKey =
        resolver.GetRequiredInputKey(step, "selection");
    const std::string statusKey = resolver.GetRequiredOutputKey(step, "status");

    // Get action parameter (optional, default "play")
    std::string action = "play";
    auto actionIt      = step.parameters.find("action");
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
        status = DispatchMediaSelection(*audioService_, logger_.get(), action,
                                        *selection);
    }

    context.Set(statusKey, status);
}

}  // namespace sdl3cpp::services::impl
