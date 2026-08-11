#include "services/interfaces/workflow/rendering/workflow_postfx_taa_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace sdl3cpp::services::impl {

// Halton sequence for sub-pixel jitter (low discrepancy, covers pixel well)
static float halton(int index, int base) {
    float f = 1.0f;
    float r = 0.0f;
    int i = index;
    while (i > 0) {
        f /= static_cast<float>(base);
        r += f * (i % base);
        i /= base;
    }
    return r;
}

WorkflowPostfxTaaStep::WorkflowPostfxTaaStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxTaaStep::GetPluginId() const {
    return "postfx.taa";
}

void WorkflowPostfxTaaStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* hdrTex = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    if (!cmd || !device || !hdrTex) return;

    WorkflowStepParameterResolver params;
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    float blendFactor = getNum("blend_factor", 0.05f);

    uint32_t w = context.Get<uint32_t>("postfx_hdr_width", 0u);
    uint32_t h = context.Get<uint32_t>("postfx_hdr_height", 0u);
    if (w == 0 || h == 0) return;

    // Track frame count for jitter sequence
    double frameCount = context.Get<double>("taa_frame_count", 0.0);
    frameCount += 1.0;
    context.Set<double>("taa_frame_count", frameCount);
    int frameIdx = static_cast<int>(frameCount);

    // --- Apply jitter to projection matrix for NEXT frame ---
    // This ensures the current frame was rendered with jitter applied last frame
    auto projMatrix = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    float jitterX = (halton(frameIdx % 16 + 1, 2) - 0.5f) / static_cast<float>(w);
    float jitterY = (halton(frameIdx % 16 + 1, 3) - 0.5f) / static_cast<float>(h);
    glm::mat4 jitteredProj = projMatrix;
    jitteredProj[2][0] += jitterX * 2.0f;
    jitteredProj[2][1] += jitterY * 2.0f;
    context.Set<glm::mat4>("render.proj_matrix", jitteredProj);

    // --- Create or get TAA pipeline ---
    auto* taaPipeline = context.Get<SDL_GPUGraphicsPipeline*>("postfx_taa_pipeline", nullptr);
    if (!taaPipeline) {
        // Need TAA shader - look for compiled shader in context
        auto* taaShader = context.Get<SDL_GPUShader*>("taa_fragment_shader", nullptr);
        auto* fullscreenVert = context.Get<SDL_GPUShader*>("postfx_vertex_shader", nullptr);
        if (!taaShader || !fullscreenVert) {
            // Shaders not compiled yet - skip TAA this frame
            return;
        }

        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = fullscreenVert;
        pipelineInfo.fragment_shader = taaShader;
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        SDL_GPUColorTargetDescription colorDesc = {};
        colorDesc.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorDesc;

        taaPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
        if (taaPipeline) {
            context.Set<SDL_GPUGraphicsPipeline*>("postfx_taa_pipeline", taaPipeline);
        } else {
            return;
        }
    }

    // --- Create or get history textures (ping-pong) ---
    auto* historyA = context.Get<SDL_GPUTexture*>("taa_history_a", nullptr);
    auto* historyB = context.Get<SDL_GPUTexture*>("taa_history_b", nullptr);
    auto taaW = context.Get<uint32_t>("taa_width", 0u);
    auto taaH = context.Get<uint32_t>("taa_height", 0u);

    if (!historyA || !historyB || taaW != w || taaH != h) {
        if (historyA) SDL_ReleaseGPUTexture(device, historyA);
        if (historyB) SDL_ReleaseGPUTexture(device, historyB);

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        texInfo.width = w;
        texInfo.height = h;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

        historyA = SDL_CreateGPUTexture(device, &texInfo);
        historyB = SDL_CreateGPUTexture(device, &texInfo);
        context.Set<SDL_GPUTexture*>("taa_history_a", historyA);
        context.Set<SDL_GPUTexture*>("taa_history_b", historyB);
        context.Set<uint32_t>("taa_width", w);
        context.Set<uint32_t>("taa_height", h);
        context.Set<bool>("taa_ping", true);
    }

    // Ping-pong: read from one history, write to the other
    bool ping = context.GetBool("taa_ping", true);
    SDL_GPUTexture* historyRead = ping ? historyA : historyB;
    SDL_GPUTexture* historyWrite = ping ? historyB : historyA;
    context.Set<bool>("taa_ping", !ping);

    auto* sampler = context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);
    if (!sampler) return;

    // --- TAA resolve pass: current HDR + history → historyWrite ---
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = historyWrite;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return;

    SDL_BindGPUGraphicsPipeline(pass, taaPipeline);

    SDL_GPUTextureSamplerBinding bindings[2] = {};
    bindings[0].texture = hdrTex;
    bindings[0].sampler = sampler;
    bindings[1].texture = historyRead;
    bindings[1].sampler = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);

    struct { float params[4]; } uniforms;
    uniforms.params[0] = blendFactor;
    uniforms.params[1] = 1.0f / static_cast<float>(w);
    uniforms.params[2] = 1.0f / static_cast<float>(h);
    uniforms.params[3] = static_cast<float>(frameCount);
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);

    // Replace the HDR texture with the TAA result for downstream post-FX
    context.Set<SDL_GPUTexture*>("postfx_hdr_texture", historyWrite);
}

}  // namespace sdl3cpp::services::impl
