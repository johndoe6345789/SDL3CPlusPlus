#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_water.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.water.map
 *
 * What fills the water. Once, water_file's quads are rasterised into a
 * 1024 x 1536 grid of surface heights over the map (Gta5WaterGrid) and
 * uploaded as gta5.water.map, which the model, terrain and emissive
 * shaders read to sink what lies under water into its colour. Every
 * frame, how far the camera is under the water over it goes in
 * gta5.camera.underwater: below 0 when it is above, or dry. Runs before
 * gta5.tiles.cull, whose flush uploads the grid.
 */
class WorkflowGta5WaterMapStep final : public IWorkflowStep {
public:
    WorkflowGta5WaterMapStep(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Build(const WorkflowStepDefinition& step, SDL_GPUDevice* device);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::vector<Gta5WaterQuad> water_;
    SDL_GPUTexture* map_{nullptr};
    SDL_GPUSampler* sampler_{nullptr};
    bool tried_{false};
};

}  // namespace sdl3cpp::services::impl
