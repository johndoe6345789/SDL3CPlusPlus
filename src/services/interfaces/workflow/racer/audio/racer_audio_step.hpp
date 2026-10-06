#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/audio/racer_audio_mixer.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.audio.update
 *
 * Plays the race's sound through RacerAudioMixer, opening it on the
 * first frame after racer.world.load. Silent when SDL3CPP_HEADLESS or
 * RACER_MUTE is set (dev runs must not make noise), or without an audio
 * device.
 */
class WorkflowRacerAudioStep final : public IWorkflowStep {
public:
    WorkflowRacerAudioStep(std::shared_ptr<ILogger> logger,
                           std::shared_ptr<RacerWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    std::unique_ptr<RacerAudioMixer> mixer_;
    bool tried_ = false;
};

}  // namespace sdl3cpp::services::impl
