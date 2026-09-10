#include "services/interfaces/workflow/rendering/workflow_postfx_ssao_step.hpp"
#include "services/interfaces/workflow/rendering/postfx_ssao_resources.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowPostfxSsaoStep::WorkflowPostfxSsaoStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxSsaoStep::GetPluginId() const {
    return "postfx.ssao";
}

void WorkflowPostfxSsaoStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (context.GetBool("frame_skip", false)) return;

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "postfx_ssao_pipeline", nullptr);
    auto* depthTex =
        context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    auto* nearestSampler =
        context.Get<SDL_GPUSampler*>("postfx_nearest_sampler", nullptr);

    if (!cmd || !device || !pipeline || !depthTex || !nearestSampler) {
        if (logger_) {
            logger_->Warn("postfx.ssao: Missing required resources, skipping");
        }
        return;
    }

    const auto fw = context.Get<uint32_t>("frame_width", 0u);
    const auto fh = context.Get<uint32_t>("frame_height", 0u);
    if (fw == 0 || fh == 0) return;

    auto* ssaoTex = GetOrCreateSsaoTexture(context, device, fw, fh);

    const auto proj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    const auto* kernelPtr =
        context.TryGet<std::vector<float>>("ssao_kernel");

    SSAOUniformData uniforms = {};
    if (!kernelPtr ||
        !BuildSsaoUniforms(proj, fw, fh, *kernelPtr, uniforms)) {
        return;
    }

    // Begin SSAO render pass
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = ssaoTex;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
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
