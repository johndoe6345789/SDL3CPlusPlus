#include "services/interfaces/workflow/fs2024/fs2024_terrain_draw_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_frustum.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_uniforms.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024TerrainDrawStep::WorkflowFs2024TerrainDrawStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TerrainState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024TerrainDrawStep::GetPluginId() const {
    return "fs2024.terrain.draw";
}

void WorkflowFs2024TerrainDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->loaded) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Fs2024StringOr(step, "pipeline_key", "gpu_pipeline_fs2024_terrain"),
        nullptr);
    const std::string texture = Fs2024StringOr(step, "texture", "ground_map");
    SDL_GPUTextureSamplerBinding map{};
    map.texture = context.Get<SDL_GPUTexture*>(texture + "_gpu", nullptr);
    map.sampler = context.Get<SDL_GPUSampler*>(texture + "_sampler", nullptr);
    if (!pass || !cmd || !pipeline || !map.texture || !map.sampler) {
        if (logger_ && !warned_ && pass) {
            logger_->Warn("fs2024.terrain.draw: missing pipeline or '" +
                          texture + "' texture; ground not drawn");
            warned_ = true;
        }
        return;
    }

    const Fs2024TerrainVertexUniforms vertex =
        BuildFs2024TerrainVertexUniforms(context);
    const Fs2024TerrainFragmentUniforms fragment =
        BuildFs2024TerrainFragmentUniforms(step, context);
    const Fs2024Frustum frustum = MakeFs2024Frustum(vertex.viewProj);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
    SDL_BindGPUFragmentSamplers(pass, 0, &map, 1);

    for (const Fs2024TerrainChunkGpu& chunk : state_->chunks) {
        if (!Fs2024BoxVisible(frustum, chunk.min, chunk.max)) continue;
        SDL_GPUBufferBinding vb{};
        vb.buffer = chunk.vertexBuffer;
        SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
        SDL_GPUBufferBinding ib{};
        ib.buffer = chunk.indexBuffer;
        SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, chunk.indexCount, 1, 0, 0, 0);
    }
}

}  // namespace sdl3cpp::services::impl
