#include "services/interfaces/workflow/rendering/postfx_taa_pipeline.hpp"

namespace sdl3cpp::services::impl {

SDL_GPUGraphicsPipeline* GetOrCreateTaaPipeline(SDL_GPUDevice* device,
                                                WorkflowContext& context) {
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("postfx_taa_pipeline", nullptr);
    if (pipeline) return pipeline;

    auto* taaShader =
        context.Get<SDL_GPUShader*>("taa_fragment_shader", nullptr);
    auto* fullscreenVert =
        context.Get<SDL_GPUShader*>("postfx_vertex_shader", nullptr);
    if (!taaShader || !fullscreenVert) {
        // Shaders not compiled yet - skip TAA this frame.
        return nullptr;
    }

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader                     = fullscreenVert;
    pipelineInfo.fragment_shader                   = taaShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUColorTargetDescription colorDesc = {};
    colorDesc.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    pipelineInfo.target_info.num_color_targets         = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorDesc;

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (pipeline) {
        context.Set<SDL_GPUGraphicsPipeline*>("postfx_taa_pipeline", pipeline);
    }
    return pipeline;
}

}  // namespace sdl3cpp::services::impl
