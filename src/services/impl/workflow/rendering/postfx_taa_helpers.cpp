#include "services/interfaces/workflow/rendering/postfx_taa_helpers.hpp"

namespace sdl3cpp::services::impl {

float Halton(int index, int base) {
    float f = 1.0f;
    float r = 0.0f;
    int i   = index;
    while (i > 0) {
        f /= static_cast<float>(base);
        r += f * (i % base);
        i /= base;
    }
    return r;
}

void ApplyTaaProjectionJitter(WorkflowContext& context, int frameIdx,
                              uint32_t width, uint32_t height) {
    auto projMatrix =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    const float jitterX =
        (Halton(frameIdx % 16 + 1, 2) - 0.5f) / static_cast<float>(width);
    const float jitterY =
        (Halton(frameIdx % 16 + 1, 3) - 0.5f) / static_cast<float>(height);
    glm::mat4 jitteredProj = projMatrix;
    jitteredProj[2][0] += jitterX * 2.0f;
    jitteredProj[2][1] += jitterY * 2.0f;
    context.Set<glm::mat4>("render.proj_matrix", jitteredProj);
}

SDL_GPUGraphicsPipeline* GetOrCreateTaaPipeline(SDL_GPUDevice* device,
                                                WorkflowContext& context) {
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "postfx_taa_pipeline", nullptr);
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
    pipelineInfo.vertex_shader   = fullscreenVert;
    pipelineInfo.fragment_shader = taaShader;
    pipelineInfo.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUColorTargetDescription colorDesc = {};
    colorDesc.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    pipelineInfo.target_info.num_color_targets        = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorDesc;

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (pipeline) {
        context.Set<SDL_GPUGraphicsPipeline*>("postfx_taa_pipeline",
                                              pipeline);
    }
    return pipeline;
}

TaaHistoryTextures GetOrCreateTaaHistoryTextures(SDL_GPUDevice* device,
                                                 WorkflowContext& context,
                                                 uint32_t width,
                                                 uint32_t height) {
    auto* historyA = context.Get<SDL_GPUTexture*>("taa_history_a", nullptr);
    auto* historyB = context.Get<SDL_GPUTexture*>("taa_history_b", nullptr);
    const auto taaW = context.Get<uint32_t>("taa_width", 0u);
    const auto taaH = context.Get<uint32_t>("taa_height", 0u);

    if (!historyA || !historyB || taaW != width || taaH != height) {
        if (historyA) SDL_ReleaseGPUTexture(device, historyA);
        if (historyB) SDL_ReleaseGPUTexture(device, historyB);

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type                 = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format               = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        texInfo.width                = width;
        texInfo.height               = height;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels           = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                        SDL_GPU_TEXTUREUSAGE_SAMPLER;

        historyA = SDL_CreateGPUTexture(device, &texInfo);
        historyB = SDL_CreateGPUTexture(device, &texInfo);
        context.Set<SDL_GPUTexture*>("taa_history_a", historyA);
        context.Set<SDL_GPUTexture*>("taa_history_b", historyB);
        context.Set<uint32_t>("taa_width", width);
        context.Set<uint32_t>("taa_height", height);
        context.Set<bool>("taa_ping", true);
    }

    // Ping-pong: read from one history, write to the other.
    const bool ping = context.GetBool("taa_ping", true);
    context.Set<bool>("taa_ping", !ping);
    return ping ? TaaHistoryTextures{historyA, historyB}
                : TaaHistoryTextures{historyB, historyA};
}

void DrawTaaResolvePass(SDL_GPUCommandBuffer* cmd,
                        SDL_GPUGraphicsPipeline* pipeline,
                        SDL_GPUTexture* hdrTex,
                        const TaaHistoryTextures& history,
                        SDL_GPUSampler* sampler, float blendFactor,
                        uint32_t width, uint32_t height,
                        double frameCount) {
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture  = history.write;
    colorTarget.load_op  = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding bindings[2] = {};
    bindings[0].texture = hdrTex;
    bindings[0].sampler = sampler;
    bindings[1].texture = history.read;
    bindings[1].sampler = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);

    struct {
        float params[4];
    } uniforms;
    uniforms.params[0] = blendFactor;
    uniforms.params[1] = 1.0f / static_cast<float>(width);
    uniforms.params[2] = 1.0f / static_cast<float>(height);
    uniforms.params[3] = static_cast<float>(frameCount);
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
