#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.sound.play
 *
 * Plays one banked sound by name. Give it either a "sound" parameter,
 * or "sound_key" naming a context key holding the sound to play, so a
 * gameplay step can choose a sound without this step knowing about it.
 * An optional "when" input gates playback on a bool, which keeps the
 * trigger in the workflow rather than in C++.
 *
 * Reads:  q3.sound.bank, q3.sound.device
 */
class WorkflowQ3SoundPlayStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3SoundPlayStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
