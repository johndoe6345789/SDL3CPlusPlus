#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_draw_step.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_tile.hpp"

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kClassMapSize = 64;

SDL_GPUTextureSamplerBinding Binding(const WorkflowContext& context,
                                     const std::string& key) {
    return {context.Get<SDL_GPUTexture*>(key + "_gpu", nullptr),
            context.Get<SDL_GPUSampler*>(key + "_sampler", nullptr)};
}

}  // namespace

WorkflowFs2024TerrainDrawStep::WorkflowFs2024TerrainDrawStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TerrainDrawStep::GetPluginId() const {
    return "fs2024.terrain.draw";
}

void WorkflowFs2024TerrainDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* terrain = context.Get<SDL_GPUGraphicsPipeline*>(
        Fs2024StringOr(step, "pipeline_key", "gpu_pipeline_fs2024_terrain"),
        nullptr);
    auto* ground = context.Get<SDL_GPUGraphicsPipeline*>(
        Fs2024StringOr(step, "ground_pipeline_key", "gpu_pipeline_fs2024_ground"),
        nullptr);
    if (!pass || !cmd || !state_->world || state_->resident.empty()) return;
    if (!terrain || !ground) {
        if (logger_ && !warned_) {
            logger_->Warn("fs2024.terrain.draw: missing a pipeline");
            warned_ = true;
        }
        return;
    }

    const Fs2024TerrainVertexUniforms vertex =
        BuildFs2024TerrainVertexUniforms(context);
    const Fs2024TerrainFragmentUniforms lighting =
        BuildFs2024TerrainFragmentUniforms(step, context);
    const Fs2024Frustum frustum = MakeFs2024Frustum(vertex.viewProj);

    SDL_BindGPUGraphicsPipeline(pass, ground);
    const Fs2024GroundFragmentUniforms groundUniforms =
        BuildFs2024GroundFragmentUniforms(
            lighting, *state_->world,
            Fs2024NumberOr(step, "material_repeat_metres", 64.f),
            kClassMapSize);
    SDL_PushGPUFragmentUniformData(cmd, 0, &groundUniforms,
                                   sizeof(groundUniforms));
    for (const auto& [key, tile] : state_->resident) {
        DrawFs2024TileGround(pass, cmd, tile, *state_->world, vertex, frustum);
    }

    SDL_BindGPUGraphicsPipeline(pass, terrain);
    const auto wall = Binding(
        context, Fs2024StringOr(step, "building_texture", "fs2024_building"));
    const auto roof =
        Binding(context, Fs2024StringOr(step, "roof_texture", "fs2024_roof"));
    for (const auto& [key, tile] : state_->resident) {
        DrawFs2024TileBuildings(pass, cmd, tile, vertex, lighting, frustum,
                                wall, roof);
    }
}

}  // namespace sdl3cpp::services::impl
