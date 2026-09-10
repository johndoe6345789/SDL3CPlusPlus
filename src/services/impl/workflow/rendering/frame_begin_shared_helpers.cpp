#include "services/interfaces/workflow/rendering/frame_begin_shared_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

ClearColorParams ReadClearColorParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver paramResolver;
    ClearColorParams out;
    auto readParam = [&](const char* name, float& value) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                value = static_cast<float>(p->numberValue);
            }
        }
    };
    readParam("clear_r", out.r);
    readParam("clear_g", out.g);
    readParam("clear_b", out.b);
    return out;
}

AcquiredSwapchain AcquireSwapchainForFrame(SDL_GPUDevice* device,
                                           SDL_Window* window) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        return {};
    }

    SDL_GPUTexture* swapchainTex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchainTex, &sw,
                                               &sh) ||
        !swapchainTex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return {};
    }

    return AcquiredSwapchain{cmd, swapchainTex, sw, sh, true};
}

SDL_GPUTexture* GetOrResizeTexture(
    SDL_GPUDevice* device, WorkflowContext& context,
    const std::string& textureKey, const std::string& widthKey,
    const std::string& heightKey, SDL_GPUTextureFormat format,
    SDL_GPUTextureUsageFlags usage, uint32_t width, uint32_t height) {
    auto* existing       = context.Get<SDL_GPUTexture*>(textureKey, nullptr);
    const auto existingW = context.Get<uint32_t>(widthKey, 0u);
    const auto existingH = context.Get<uint32_t>(heightKey, 0u);

    if (existing && existingW == width && existingH == height) {
        return existing;
    }
    if (existing) {
        SDL_ReleaseGPUTexture(device, existing);
    }

    SDL_GPUTextureCreateInfo info = {};
    info.type                     = SDL_GPU_TEXTURETYPE_2D;
    info.format                   = format;
    info.width                    = width;
    info.height                   = height;
    info.layer_count_or_depth     = 1;
    info.num_levels               = 1;
    info.usage                    = usage;

    auto* created = SDL_CreateGPUTexture(device, &info);
    context.Set<SDL_GPUTexture*>(textureKey, created);
    context.Set<uint32_t>(widthKey, width);
    context.Set<uint32_t>(heightKey, height);
    return created;
}

SDL_GPURenderPass* BeginColorDepthRenderPass(SDL_GPUCommandBuffer* cmd,
                                             SDL_GPUTexture* colorTexture,
                                             const ClearColorParams& clear,
                                             SDL_GPUTexture* depthTexture) {
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture                = colorTexture;
    colorTarget.clear_color.r          = clear.r;
    colorTarget.clear_color.g          = clear.g;
    colorTarget.clear_color.b          = clear.b;
    colorTarget.clear_color.a          = 1.0f;
    colorTarget.load_op                = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo dsTarget = {};
    dsTarget.texture                       = depthTexture;
    dsTarget.clear_depth                   = 1.0f;
    dsTarget.load_op                       = SDL_GPU_LOADOP_CLEAR;
    dsTarget.store_op                      = SDL_GPU_STOREOP_DONT_CARE;

    return SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &dsTarget);
}

}  // namespace sdl3cpp::services::impl
