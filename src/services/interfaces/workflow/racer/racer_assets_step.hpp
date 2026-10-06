#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: racer.assets.build
 *
 * Reads an install of Star Wars Episode I Racer and writes modernised
 * copies of its assets: textures decoded from the lev01 block and
 * upscaled, UI TGAs upscaled to PNG, sounds resampled to 44.1 kHz, and
 * track splines plotted top-down.
 * The install is only read; output goes to `out_dir`.
 *
 * Parameters: `racer_dir` (or env RACER_DIR); `out_dir` (default
 * `racer_generated`; present but empty skips the export); `scale` (a
 * power of two, default 4); `audio_rate` (default 44100);
 * `track_table`. Publishes racer.texture_count, racer.image_count,
 * racer.audio_count, racer.track_count and racer.model_count.
 */
class WorkflowRacerAssetsStep final : public IWorkflowStep {
public:
    explicit WorkflowRacerAssetsStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
