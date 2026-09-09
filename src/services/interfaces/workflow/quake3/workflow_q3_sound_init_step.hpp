#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.sound.init
 *
 * Opens the audio device the other sound steps mix into. SDL3 mixes
 * every stream bound to a device, so each sound played gets its own
 * stream and overlapping sounds need no mixer of our own.
 *
 * Writes: q3.sound.device (SDL_AudioDeviceID)
 */
class WorkflowQ3SoundInitStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3SoundInitStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
