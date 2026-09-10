#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Binds `texture`/`sampler` as sampler 0, plus the context's shadow map as
/// sampler 1 when present (falling back to just the albedo when it isn't).
void BindDrawTexturedSamplers(SDL_GPURenderPass* pass,
                              const WorkflowContext& context,
                              SDL_GPUTexture* texture, SDL_GPUSampler* sampler);

/// The mesh buffers, index count, and texture/sampler resolved for one
/// draw.textured call.
struct DrawTexturedResources {
    SDL_GPUBuffer* vb       = nullptr;
    SDL_GPUBuffer* ib       = nullptr;
    uint32_t indexCount     = 0;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/// Looks up `params.meshName`'s buffers/metadata and `params.textureName`'s
/// GPU texture/sampler; logs a Warn and returns false (via `logger`, which
/// may be null) if either is missing from the context, exactly as
/// draw.textured always has.
bool ResolveDrawTexturedResources(const WorkflowContext& context,
                                  const DrawTexturedParams& params,
                                  const std::shared_ptr<ILogger>& logger,
                                  DrawTexturedResources& out);

}  // namespace sdl3cpp::services::impl
