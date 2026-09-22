#include "services/impl/workflow/stunts/render/stunts_dashboard_draw.hpp"

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

namespace sdl3cpp::services::impl {

void DrawStuntsDashboardMesh(WorkflowContext& context,
                            SDL_GPURenderPass* pass,
                            SDL_GPUCommandBuffer* cmd,
                            SDL_GPUGraphicsPipeline* pipeline,
                            SDL_GPUBuffer* vertexBuffer,
                            SDL_GPUBuffer* indexBuffer,
                            std::uint32_t indexCount) {
    auto* texture =
        context.Get<SDL_GPUTexture*>("stunts_palette_gpu", nullptr);
    auto* sampler =
        context.Get<SDL_GPUSampler*>("stunts_palette_sampler", nullptr);
    if (!texture || !sampler || indexCount == 0) return;

    rendering::VertexUniformData vertex = {};
    vertex.mvp[0] = vertex.mvp[5] = vertex.mvp[10] = vertex.mvp[15] = 1.f;
    vertex.model_mat[0] = vertex.model_mat[5] = vertex.model_mat[10] =
        vertex.model_mat[15] = 1.f;
    vertex.normal[1] = 1.f;
    vertex.uv_scale[0] = vertex.uv_scale[1] = 1.f;
    const rendering::FragmentUniformData fragment = {};

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
    const SDL_GPUTextureSamplerBinding binding = {texture, sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    SDL_GPUBufferBinding vb = {};
    vb.buffer = vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {};
    ib.buffer = indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
