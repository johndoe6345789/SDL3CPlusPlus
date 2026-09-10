#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// bsp.portal_view's lazily-created 512x512 render target, matching depth
/// buffer, and clamp/linear sampler, cached in the context under the
/// `bsp_portal_view_*` keys so later frames reuse them.
struct PortalViewTargets {
    SDL_GPUTexture* colorTex = nullptr;
    SDL_GPUTexture* depthTex = nullptr;
    SDL_GPUSampler* sampler  = nullptr;
};

/// Returns the cached targets, creating and caching any that are missing.
/// Any member left null means creation failed for that resource.
PortalViewTargets EnsurePortalViewTargets(SDL_GPUDevice* device,
                                          SDL_Window* window,
                                          WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
