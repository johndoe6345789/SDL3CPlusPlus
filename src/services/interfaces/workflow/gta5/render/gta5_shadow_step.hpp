#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_instance_batch.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.shadow.draw
 *
 * The sun's shadow map. Culls its own casters -- scenery, shadow
 * proxies, the cars and the player -- within `radius` metres of the
 * camera as the sun sees them, draws their depth into a `size`-square
 * map on a command buffer of its own, and publishes it
 * (shadow_depth_texture, shadow_depth_sampler: a comparison sampler) with
 * its view-projection (render.shadow_vp) for gta5.tiles.draw to shade
 * with. Runs after gta5.tiles.cull and before the scene is drawn.
 */
class WorkflowGta5ShadowDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5ShadowDrawStep(std::shared_ptr<ILogger> logger,
                               std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool Ensure(SDL_GPUDevice* device, int size);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5InstanceBatch casters_;
    SDL_GPUTexture* depth_{nullptr};
    SDL_GPUSampler* sampler_{nullptr};
    int size_{0};
    bool logged_{false};
};

}  // namespace sdl3cpp::services::impl
