#pragma once

#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_soundboard_state_service.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/soundboard_types.hpp"

#include <cstdint>
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowSoundboardAudioStep final : public IWorkflowStep {
public:
    WorkflowSoundboardAudioStep(std::shared_ptr<IAudioService> audioService,
                                std::shared_ptr<ISoundboardStateService> stateService,
                                std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<ISoundboardStateService> stateService_;
    std::shared_ptr<ILogger> logger_;
    std::uint64_t lastRequestId_ = 0;
};

}  // namespace sdl3cpp::services::impl
