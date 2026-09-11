#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_instance_batch.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_water.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.reflection.draw
 *
 * What the water mirrors. The camera is reflected in the water it is
 * over -- the sea, or a lake at its own height (water_file) -- and from
 * there the sky, the reflection and water proxies (GTA's stand-ins for a
 * district in its reflections), large scenery, the cars and the player
 * are drawn, clipped at the water line, at half the render size into
 * gta5.reflection.texture, the height into gta5.reflection.height, on
 * a command buffer of its own before the scene. Mirrored triangles wind
 * the other way, so it draws with the front-culling
 * gpu_pipeline_gta5_reflect and gpu_pipeline_gta5_reflect_terrain.
 */
class WorkflowGta5ReflectionDrawStep final : public IWorkflowStep {
public:
    WorkflowGta5ReflectionDrawStep(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    bool Ensure(SDL_GPUDevice* device, std::uint32_t width,
                std::uint32_t height);
    /// The mirrored sky, then the batch, into the target. Returns draws.
    int DrawMirror(const WorkflowStepDefinition& step, WorkflowContext& context,
                   SDL_GPUCommandBuffer* cmd, const glm::mat4& view,
                   const glm::mat4& proj, const glm::vec3& eye);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5InstanceBatch batch_;
    SDL_GPUTexture* colour_{nullptr};
    SDL_GPUTexture* depth_{nullptr};
    std::uint32_t width_{0};
    std::uint32_t height_{0};
    bool logged_{false};
    std::vector<Gta5WaterQuad> water_;  // where the mirror may go
    bool loaded_{false};
};

}  // namespace sdl3cpp::services::impl
