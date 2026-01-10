#pragma once

#include "../../interfaces/i_audio_service.hpp"
#include "../../interfaces/i_logger.hpp"
#include "../../interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowAudioPlayStep final : public IWorkflowStep {
public:
    WorkflowAudioPlayStep(std::shared_ptr<IAudioService> audioService,
                          std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
