#include "services/interfaces/workflow/rendering/workflow_frame_begin_offscreen_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameBeginOffscreenStep::WorkflowFrameBeginOffscreenStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameBeginOffscreenStep::GetPluginId() const {
    return "frame.gpu.begin_offscreen";
}

void WorkflowFrameBeginOffscreenStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error("frame.gpu.begin_offscreen: No GPU device or window in context");
    }

    // Read clear color from parameters
    WorkflowStepParameterResolver paramResolver;
    float bgR = 0.1f, bgG = 0.1f, bgB = 0.15f;
    auto readParam = [&](const char* name, auto& out) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                out = static_cast<std::remove_reference_t<decltype(out)>>(p->numberValue);
            }
        }
    };
    readParam("clear_r", bgR);
    readParam("clear_g", bgG);
    readParam("clear_b", bgB);

    // Acquire command buffer
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        context.Set<bool>("frame_skip", true);
        return;
    }

    // Acquire swapchain texture (stored for composite step later)
    SDL_GPUTexture* swapchainTex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTex, &sw, &sh) || !swapchainTex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Set<bool>("frame_skip", true);
        return;
    }

    // Store swapchain texture for the composite step
    context.Set<SDL_GPUTexture*>("postfx_swapchain_texture", swapchainTex);

    // Create or resize HDR color texture
    auto* existingHdr = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    auto hdrW = context.Get<uint32_t>("postfx_hdr_width", 0u);
    auto hdrH = context.Get<uint32_t>("postfx_hdr_height", 0u);

    if (!existingHdr || hdrW != sw || hdrH != sh) {
        if (existingHdr) {
            SDL_ReleaseGPUTexture(device, existingHdr);
        }
        SDL_GPUTextureCreateInfo hdrInfo = {};
        hdrInfo.type = SDL_GPU_TEXTURETYPE_2D;
        hdrInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        hdrInfo.width = sw;
        hdrInfo.height = sh;
        hdrInfo.layer_count_or_depth = 1;
        hdrInfo.num_levels = 1;
        hdrInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

        existingHdr = SDL_CreateGPUTexture(device, &hdrInfo);
        context.Set<SDL_GPUTexture*>("postfx_hdr_texture", existingHdr);
        context.Set<uint32_t>("postfx_hdr_width", sw);
        context.Set<uint32_t>("postfx_hdr_height", sh);
    }

    // Create or resize depth texture
    auto* existingDepth = context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    auto existingDW = context.Get<uint32_t>("gpu_depth_width", 0u);
    auto existingDH = context.Get<uint32_t>("gpu_depth_height", 0u);

    if (!existingDepth || existingDW != sw || existingDH != sh) {
        if (existingDepth) {
            SDL_ReleaseGPUTexture(device, existingDepth);
        }
        SDL_GPUTextureCreateInfo depthInfo = {};
        depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depthInfo.width = sw;
        depthInfo.height = sh;
        depthInfo.layer_count_or_depth = 1;
        depthInfo.num_levels = 1;
        depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

        existingDepth = SDL_CreateGPUTexture(device, &depthInfo);
        context.Set<SDL_GPUTexture*>("gpu_depth_texture", existingDepth);
        context.Set<uint32_t>("gpu_depth_width", sw);
        context.Set<uint32_t>("gpu_depth_height", sh);
    }

    // Begin offscreen render pass targeting HDR texture + depth
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = existingHdr;
    colorTarget.clear_color.r = bgR;
    colorTarget.clear_color.g = bgG;
    colorTarget.clear_color.b = bgB;
    colorTarget.clear_color.a = 1.0f;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo dsTarget = {};
    dsTarget.texture = existingDepth;
    dsTarget.clear_depth = 1.0f;
    dsTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    dsTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &dsTarget);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Set<bool>("frame_skip", true);
        return;
    }

    // Store for subsequent draw steps
    context.Set<SDL_GPUCommandBuffer*>("gpu_command_buffer", cmd);
    context.Set<SDL_GPURenderPass*>("gpu_render_pass", pass);
    context.Set<bool>("frame_skip", false);
    context.Set<uint32_t>("frame_width", sw);
    context.Set<uint32_t>("frame_height", sh);
}

}  // namespace sdl3cpp::services::impl
