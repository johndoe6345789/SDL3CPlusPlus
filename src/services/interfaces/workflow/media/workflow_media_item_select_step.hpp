#pragma once

#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/media_types.hpp"

#include <cstdint>
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Generic step: Play or process selected media item
 *
 * Parametric, reusable step that handles ANY media selection
 * (audio playback, image display, video playback, etc.)
 *
 * Parameters:
 *   action: "play" | "load" | "process" (defines what to do with selection)
 *   selection_input_key: Context key where MediaSelection is stored
 *   status_output_key: Context key where status message is stored
 *
 * Inputs (context):
 *   MediaSelection at selection_input_key
 *
 * Outputs (context):
 *   status message at status_output_key
 */
class WorkflowMediaItemSelectStep final : public IWorkflowStep {
public:
    WorkflowMediaItemSelectStep(std::shared_ptr<IAudioService> audioService,
                                std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<ILogger> logger_;
    std::uint64_t lastRequestId_ = 0;
};

}  // namespace sdl3cpp::services::impl
