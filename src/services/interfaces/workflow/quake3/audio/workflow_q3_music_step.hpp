#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.music.play
 *
 * Plays a background track the way ioq3 does with
 * S_StartBackgroundTrack: an optional intro is played once, then a
 * second file loops for as long as the map is running.
 *
 * SDL streams do not loop by themselves, so this step also tops the
 * stream up whenever it runs low, which is why it belongs in the frame
 * rather than only at map load.
 *
 * Reads:  bsp_config (pk3_path), q3.sound.device, q3.sound.device_spec
 * Params: intro, loop (paths inside the pk3), volume
 */
class WorkflowQ3MusicStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MusicStep(std::shared_ptr<ILogger> logger);
    ~WorkflowQ3MusicStep() override;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool Start(const WorkflowStepDefinition& step, WorkflowContext& context);

    std::shared_ptr<ILogger> logger_;
    SDL_AudioStream* stream_{nullptr};
    std::vector<uint8_t> loopPcm_;
};

}  // namespace sdl3cpp::services::impl
