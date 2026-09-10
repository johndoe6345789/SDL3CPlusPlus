#include "services/interfaces/workflow/rendering/workflow_postfx_taa_step.hpp"
#include "services/interfaces/workflow/rendering/postfx_taa_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowPostfxTaaStep::WorkflowPostfxTaaStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxTaaStep::GetPluginId() const {
    return "postfx.taa";
}

void WorkflowPostfxTaaStep::Execute(const WorkflowStepDefinition& step,
                                    WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* hdrTex = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    if (!cmd || !device || !hdrTex) return;

    WorkflowStepParameterResolver params;
    const auto* p = params.FindParameter(step, "blend_factor");
    const float blendFactor =
        (p && p->type == WorkflowParameterValue::Type::Number)
            ? static_cast<float>(p->numberValue)
            : 0.05f;

    const auto w = context.Get<uint32_t>("postfx_hdr_width", 0u);
    const auto h = context.Get<uint32_t>("postfx_hdr_height", 0u);
    if (w == 0 || h == 0) return;

    // Track frame count for the jitter sequence.
    double frameCount = context.Get<double>("taa_frame_count", 0.0);
    frameCount += 1.0;
    context.Set<double>("taa_frame_count", frameCount);
    const int frameIdx = static_cast<int>(frameCount);

    // Jitter is for the NEXT frame: the current frame was rendered with
    // last frame's jitter already applied.
    ApplyTaaProjectionJitter(context, frameIdx, w, h);

    auto* taaPipeline = GetOrCreateTaaPipeline(device, context);
    if (!taaPipeline) return;

    const TaaHistoryTextures history =
        GetOrCreateTaaHistoryTextures(device, context, w, h);

    auto* sampler =
        context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);
    if (!sampler) return;

    DrawTaaResolvePass(cmd, taaPipeline, hdrTex, history, sampler, blendFactor,
                       w, h, frameCount);

    // Replace the HDR texture with the TAA result for downstream post-FX.
    context.Set<SDL_GPUTexture*>("postfx_hdr_texture", history.write);
}

}  // namespace sdl3cpp::services::impl
