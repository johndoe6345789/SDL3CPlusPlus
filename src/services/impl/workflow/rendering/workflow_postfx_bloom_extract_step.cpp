#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_extract_step.hpp"
#include "services/interfaces/workflow/rendering/bloom_ping_pong.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxBloomExtractStep::WorkflowPostfxBloomExtractStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxBloomExtractStep::GetPluginId() const {
    return "postfx.bloom_extract";
}

void WorkflowPostfxBloomExtractStep::Execute(const WorkflowStepDefinition&,
                                             WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device   = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "postfx_bloom_extract_pipeline", nullptr);
    auto* hdrTex = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    auto* sampler =
        context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);

    if (!cmd || !device || !pipeline || !hdrTex || !sampler) {
        if (logger_) {
            logger_->Warn(
                "postfx.bloom_extract: Missing required resources, "
                "skipping");
        }
        return;
    }

    auto fw = context.Get<uint32_t>("frame_width", 0u);
    auto fh = context.Get<uint32_t>("frame_height", 0u);
    if (fw == 0 || fh == 0) return;

    uint32_t halfW = fw / 2;
    uint32_t halfH = fh / 2;
    if (halfW == 0) halfW = 1;
    if (halfH == 0) halfH = 1;

    const BloomPingPongTextures textures =
        EnsureBloomPingPongTextures(device, context, halfW, halfH);

    DrawBloomExtractPass(cmd, pipeline, hdrTex, sampler, textures.ping);
}

}  // namespace sdl3cpp::services::impl
