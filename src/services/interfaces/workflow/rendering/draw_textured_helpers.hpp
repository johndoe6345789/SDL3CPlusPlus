#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// draw.textured's resolved parameters, each with the original defaults.
struct DrawTexturedParams {
    std::string meshName    = "plane";
    std::string textureName = "texture";
    std::string facing;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
    float scale     = 1.0f;
    float roughness = 0.8f;
    float metallic  = 0.0f;
};

DrawTexturedParams ReadDrawTexturedParams(const WorkflowStepDefinition& step);

/// The model matrix and outward surface normal for one draw.textured call.
struct DrawTexturedTransform {
    glm::mat4 model;
    glm::vec3 normal;
};

/**
 * @brief Builds the model matrix and surface normal for `params`.
 *
 * With a non-empty `facing` ("up"/"down"/"north"/"south"/"east"/"west"),
 * orients the plane to face that direction at `pos_*` and derives the
 * normal from it; `rot_*` is ignored in that mode, matching the original.
 * With no `facing`, applies `pos_*`/`rot_*`/`scale` in that order and the
 * normal is always +Y.
 */
DrawTexturedTransform BuildDrawTexturedTransform(
    const DrawTexturedParams& params);

/// Fills the vertex/fragment uniforms shared by this draw call from the
/// context's pre-computed camera/shadow/lighting state plus this call's
/// model matrix, normal, and material parameters.
void BuildDrawTexturedUniforms(const WorkflowContext& context,
                               const DrawTexturedTransform& transform,
                               float roughness, float metallic,
                               rendering::VertexUniformData& vu,
                               rendering::FragmentUniformData& fu);

/// Binds `texture`/`sampler` as sampler 0, plus the context's shadow map as
/// sampler 1 when present (falling back to just the albedo when it isn't).
void BindDrawTexturedSamplers(SDL_GPURenderPass* pass,
                              const WorkflowContext& context,
                              SDL_GPUTexture* texture, SDL_GPUSampler* sampler);

/// The mesh buffers, index count, and texture/sampler resolved for one
/// draw.textured call.
struct DrawTexturedResources {
    SDL_GPUBuffer* vb   = nullptr;
    SDL_GPUBuffer* ib   = nullptr;
    uint32_t indexCount = 0;
    SDL_GPUTexture* texture  = nullptr;
    SDL_GPUSampler* sampler  = nullptr;
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
