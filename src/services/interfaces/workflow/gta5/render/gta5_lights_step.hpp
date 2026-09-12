#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.lights.draw
 *
 * GTA's distant lights -- 51,000 far-off street lamps and lit windows
 * from lights_dir (lodlights.rpf's distlodlights_*.ymap.xml) -- as soft
 * points of light turned to face the camera, faded in with the night
 * (gta5.time.night). Near the camera, where GTA would light the lamps
 * themselves, they fade out between fade_from and fade_to metres. Drawn
 * after the water with gpu_pipeline_gta5_lights: blended, not
 * depth-written. Loaded once, all of them: they are only positions.
 */
class WorkflowGta5LightsDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5LightsDrawStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Load(const WorkflowStepDefinition& step, SDL_GPUDevice* device);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    SDL_GPUBuffer* vertices_{nullptr};
    std::uint32_t count_{0};
    bool tried_{false};
    bool staged_{false};  // copied with the next frame's uploads
};

}  // namespace sdl3cpp::services::impl
