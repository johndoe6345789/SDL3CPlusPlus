#include "services/interfaces/workflow/fs2024/terrain/fs2024_vectors_draw_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_chunk.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_tile.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_drawn_tiles.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// Matches WaterVertex in fs2024_water.vert.
struct WaterVertex {
    glm::mat4 viewProj{1.f};
    glm::vec4 originOffset{0.f};
};

SDL_GPUGraphicsPipeline* Pipeline(const WorkflowStepDefinition& step,
                                  const WorkflowContext& context,
                                  const char* param, const char* fallback) {
    return context.Get<SDL_GPUGraphicsPipeline*>(
        Fs2024StringOr(step, param, fallback), nullptr);
}

}  // namespace

WorkflowFs2024VectorsDrawStep::WorkflowFs2024VectorsDrawStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024VectorsDrawStep::GetPluginId() const {
    return "fs2024.vectors.draw";
}

void WorkflowFs2024VectorsDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->world) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* roads = Pipeline(step, context, "road_pipeline_key",
                           "gpu_pipeline_fs2024_road");
    auto* water = Pipeline(step, context, "water_pipeline_key",
                           "gpu_pipeline_fs2024_water");
    const Fs2024World& world = *state_->world;
    const SDL_GPUTextureSamplerBinding asphalt = {world.roadTexture,
                                                  world.roadSampler};
    if (!pass || !cmd || !roads || !water || !asphalt.texture) return;

    Fs2024TerrainVertexUniforms vertex =
        BuildFs2024TerrainVertexUniforms(context);
    const Fs2024Frustum frustum = MakeFs2024Frustum(vertex.viewProj);
    const auto lighting = BuildFs2024TerrainFragmentUniforms(step, context);
    SDL_BindGPUGraphicsPipeline(pass, roads);
    SDL_PushGPUFragmentUniformData(cmd, 0, &lighting, sizeof(lighting));
    SDL_BindGPUFragmentSamplers(pass, 0, &asphalt, 1);
    ForEachDrawnFs2024Tile(*state_, [&](const Fs2024LoadedTile& tile) {
        vertex.originOffset = glm::vec4(tile.offset, 0.f);
        SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
        DrawFs2024Chunk(pass, tile.roadChunk, frustum, tile.offset);
    });

    // The water's mirror slot takes any texture while there is none.
    const Gta5WaterUniforms shading = BuildGta5WaterUniforms(context, false);
    SDL_BindGPUGraphicsPipeline(pass, water);
    SDL_PushGPUFragmentUniformData(cmd, 0, &shading, sizeof(shading));
    SDL_BindGPUFragmentSamplers(pass, 0, &asphalt, 1);
    WaterVertex placed{vertex.viewProj, glm::vec4(0.f)};
    ForEachDrawnFs2024Tile(*state_, [&](const Fs2024LoadedTile& tile) {
        placed.originOffset = glm::vec4(tile.offset, 0.f);
        SDL_PushGPUVertexUniformData(cmd, 0, &placed, sizeof(placed));
        DrawFs2024Chunk(pass, tile.waterChunk, frustum, tile.offset);
    });
}

}  // namespace sdl3cpp::services::impl
