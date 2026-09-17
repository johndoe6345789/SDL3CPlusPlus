#include "services/interfaces/workflow/fs2024/fs2024_terrain_draw_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_frustum.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_uniforms.hpp"

#include <string>
#include <unordered_map>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void DrawIndexed(SDL_GPURenderPass* pass, SDL_GPUBuffer* vertexBuffer,
                 SDL_GPUBuffer* indexBuffer, std::uint32_t indexCount) {
    SDL_GPUBufferBinding vb{};
    vb.buffer = vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib{};
    ib.buffer = indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
}

void DrawChunk(SDL_GPURenderPass* pass, const Fs2024TerrainChunkGpu& chunk,
              const Fs2024Frustum& frustum) {
    if (!Fs2024BoxVisible(frustum, chunk.min, chunk.max)) return;
    DrawIndexed(pass, chunk.vertexBuffer, chunk.indexBuffer,
               chunk.indexCount);
}

/// A landmark's own groups have no per-group bounds to cull by, unlike
/// a tile's ground/building chunks -- fine while a bake only ever
/// places a handful of these, unlike the hundreds of ground blocks
/// frustum culling matters for.
void DrawLandmarks(
    SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
    const Fs2024LoadedTile& tile,
    const std::unordered_map<std::string, Fs2024LandmarkKitGpu>& kits,
    Fs2024TerrainFragmentUniforms fragment) {
    fragment.runway = glm::vec4(0.f);
    for (const Fs2024LandmarkInstance& instance : tile.landmarks) {
        const auto it = kits.find(instance.model);
        if (it == kits.end()) continue;
        for (const Fs2024LandmarkGroupGpu& group : it->second.groups) {
            if (group.indexCount == 0 || !group.texture) continue;
            SDL_PushGPUFragmentUniformData(cmd, 0, &fragment,
                                          sizeof(fragment));
            SDL_GPUTextureSamplerBinding tex{group.texture, group.sampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &tex, 1);
            DrawIndexed(pass, group.vertexBuffer, group.indexBuffer,
                       group.indexCount);
        }
    }
}

/// Falls back to whichever tile has no texture of its own: a plain
/// grey rather than a missing-sampler no-op draw. Buildings draw
/// through the same pipeline with the shared plain texture `building`
/// and no runway overlay -- they are massing, not an airport surface.
void DrawOneTile(
    SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
    const Fs2024LoadedTile& tile, const Fs2024Frustum& frustum,
    Fs2024TerrainFragmentUniforms fragment,
    SDL_GPUTextureSamplerBinding building,
    const std::unordered_map<std::string, Fs2024LandmarkKitGpu>& kits) {
    if (tile.groundTexture && tile.groundSampler) {
        fragment.runway = tile.runway;
        fragment.runwayAxis = tile.runwayAxis;
        SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
        SDL_GPUTextureSamplerBinding ground{tile.groundTexture,
                                            tile.groundSampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &ground, 1);
        for (const Fs2024TerrainChunkGpu& chunk : tile.terrain.chunks) {
            DrawChunk(pass, chunk, frustum);
        }
    }

    if (tile.buildingChunk.indexCount > 0 && building.texture) {
        fragment.runway = glm::vec4(0.f);
        SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
        SDL_BindGPUFragmentSamplers(pass, 0, &building, 1);
        DrawChunk(pass, tile.buildingChunk, frustum);
    }

    if (!tile.landmarks.empty()) {
        DrawLandmarks(pass, cmd, tile, kits, fragment);
    }
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
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Fs2024StringOr(step, "pipeline_key", "gpu_pipeline_fs2024_terrain"),
        nullptr);
    if (!pass || !cmd || !pipeline || state_->resident.empty()) {
        if (logger_ && !warned_ && pass && !pipeline) {
            logger_->Warn("fs2024.terrain.draw: no pipeline; ground not "
                          "drawn");
            warned_ = true;
        }
        return;
    }

    const Fs2024TerrainVertexUniforms vertex =
        BuildFs2024TerrainVertexUniforms(context);
    const Fs2024TerrainFragmentUniforms fragment =
        BuildFs2024TerrainFragmentUniforms(step, context);
    const Fs2024Frustum frustum = MakeFs2024Frustum(vertex.viewProj);
    const std::string buildingKey =
        Fs2024StringOr(step, "building_texture", "fs2024_building");
    SDL_GPUTextureSamplerBinding building{
        context.Get<SDL_GPUTexture*>(buildingKey + "_gpu", nullptr),
        context.Get<SDL_GPUSampler*>(buildingKey + "_sampler", nullptr)};

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));

    for (const auto& [key, tile] : state_->resident) {
        DrawOneTile(pass, cmd, tile, frustum, fragment, building,
                   state_->landmarkKits);
    }
}

}  // namespace sdl3cpp::services::impl
