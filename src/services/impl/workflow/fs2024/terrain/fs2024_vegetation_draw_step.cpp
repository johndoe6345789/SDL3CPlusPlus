#include "services/interfaces/workflow/fs2024/terrain/fs2024_vegetation_draw_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_tile.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_drawn_tiles.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024VegetationDrawStep::WorkflowFs2024VegetationDrawStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024VegetationDrawStep::GetPluginId() const {
    return "fs2024.vegetation.draw";
}

void WorkflowFs2024VegetationDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->world) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Fs2024StringOr(step, "pipeline_key", "gpu_pipeline_fs2024_vegetation"),
        nullptr);
    if (!pass || !cmd || !pipeline) return;

    Fs2024TerrainVertexUniforms vertex =
        BuildFs2024TerrainVertexUniforms(context);
    const Fs2024Frustum frustum = MakeFs2024Frustum(vertex.viewProj);
    const auto lighting = BuildFs2024TerrainFragmentUniforms(step, context);
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUFragmentUniformData(cmd, 0, &lighting, sizeof(lighting));

    // The tile itself was already frustum-tested when it was chosen to
    // draw (ForEachDrawnFs2024Tile); its own vegetation is small enough
    // next to that that a second, per-chunk test is not worth doing.
    const auto& species = state_->vegetationSpecies;
    ForEachDrawnFs2024Tile(*state_, [&](const Fs2024LoadedTile& tile) {
        vertex.originOffset = glm::vec4(tile.offset, 0.f);
        for (const Fs2024VegetationChunkGpu& chunk : tile.vegetationChunks) {
            const auto found = species.find(chunk.speciesName);
            if (found == species.end() || !found->second.texture) continue;
            SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
            const SDL_GPUTextureSamplerBinding binding = {
                found->second.texture, found->second.sampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
            SDL_GPUBufferBinding vb{};
            vb.buffer = chunk.vertexBuffer;
            SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
            SDL_GPUBufferBinding ib{};
            ib.buffer = chunk.indexBuffer;
            SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_DrawGPUIndexedPrimitives(pass, chunk.indexCount, 1, 0, 0, 0);
        }
    });
}

}  // namespace sdl3cpp::services::impl
