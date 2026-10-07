#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_panel.hpp"
#include "services/interfaces/workflow/racer/video/racer_video_decoder.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <SDL3/SDL_audio.h>

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.video.play
 *
 * Plays the install's cutscenes while the flow is in its Cutscene phase:
 * the LucasArts logo, the opening crawl and intro on the first run, and
 * each planet's flyover before the tournament first races there. The
 * 640 x 272 film is decoded with FFmpeg, filtered up to fill the width
 * of the screen (letterboxed, as in the original), and played with its
 * sound; select or back skips to the next. Silent when SDL3CPP_HEADLESS
 * or RACER_MUTE is set. Runs after racer.screen.draw. Parameter:
 * racer_dir (env RACER_DIR).
 */
class WorkflowRacerVideoStep final : public IWorkflowStep {
public:
    WorkflowRacerVideoStep(std::shared_ptr<ILogger> logger,
                           std::shared_ptr<RacerWorldState> state);
    ~WorkflowRacerVideoStep() override;
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool StartNext(const WorkflowStepDefinition& step, SDL_GPUDevice* device,
                   SDL_GPUCommandBuffer* cmd, float aspect);
    void Stop();
    void FeedAudio();

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    std::unique_ptr<RacerVideoDecoder> decoder_;
    RacerPanel panel_;
    std::vector<std::uint8_t> rgba_;
    SDL_AudioStream* audio_ = nullptr;
    double clock_ = 0.0;     ///< seconds into the current video
    int shown_ = 0;          ///< frames decoded so far
    std::string playing_;
};

}  // namespace sdl3cpp::services::impl
