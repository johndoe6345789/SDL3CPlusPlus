#include "services/interfaces/workflow/rendering/workflow_postfx_ssao_step.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

// Must match SSAOParams in postfx_ssao.frag.metal
struct SSAOUniformData {
    float projection[16];
    float inv_projection[16];
    float params[4];       // radius, bias, 1/width, 1/height
    float kernel[16 * 4];  // 16 float4 samples
};

WorkflowPostfxSsaoStep::WorkflowPostfxSsaoStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxSsaoStep::GetPluginId() const {
    return "postfx.ssao";
}

void WorkflowPostfxSsaoStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (context.GetBool("frame_skip", false)) return;

    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("postfx_ssao_pipeline", nullptr);
    auto* depthTex = context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    auto* nearestSampler = context.Get<SDL_GPUSampler*>("postfx_nearest_sampler", nullptr);

    if (!cmd || !device || !pipeline || !depthTex || !nearestSampler) {
        if (logger_) logger_->Warn("postfx.ssao: Missing required resources, skipping");
        return;
    }

    auto fw = context.Get<uint32_t>("frame_width", 0u);
    auto fh = context.Get<uint32_t>("frame_height", 0u);
    if (fw == 0 || fh == 0) return;

    // Create or resize SSAO output texture (R8_UNORM)
    auto* ssaoTex = context.Get<SDL_GPUTexture*>("postfx_ssao_texture", nullptr);
    auto ssaoW = context.Get<uint32_t>("postfx_ssao_width", 0u);
    auto ssaoH = context.Get<uint32_t>("postfx_ssao_height", 0u);

    if (!ssaoTex || ssaoW != fw || ssaoH != fh) {
        if (ssaoTex) SDL_ReleaseGPUTexture(device, ssaoTex);

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
        texInfo.width = fw;
        texInfo.height = fh;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

        ssaoTex = SDL_CreateGPUTexture(device, &texInfo);
        context.Set<SDL_GPUTexture*>("postfx_ssao_texture", ssaoTex);
        context.Set<uint32_t>("postfx_ssao_width", fw);
        context.Set<uint32_t>("postfx_ssao_height", fh);
    }

    // Build uniform data
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    glm::mat4 invProj = glm::inverse(proj);

    const auto* kernelPtr = context.TryGet<std::vector<float>>("ssao_kernel");
    if (!kernelPtr || kernelPtr->size() < 64) return;

    SSAOUniformData uniforms = {};
    std::memcpy(uniforms.projection, glm::value_ptr(proj), 64);
    std::memcpy(uniforms.inv_projection, glm::value_ptr(invProj), 64);
    uniforms.params[0] = 0.5f;   // radius
    uniforms.params[1] = 0.025f; // bias
    uniforms.params[2] = 1.0f / float(fw);
    uniforms.params[3] = 1.0f / float(fh);
    std::memcpy(uniforms.kernel, kernelPtr->data(), 16 * 4 * sizeof(float));

    // Begin SSAO render pass
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = ssaoTex;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    // Bind depth texture with nearest sampler
    SDL_GPUTextureSamplerBinding depthBinding = {};
    depthBinding.texture = depthTex;
    depthBinding.sampler = nearestSampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &depthBinding, 1);

    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
