#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace sdl3cpp::services::impl {

/// draw.viewmodel's tunable parameters, each with the original defaults.
struct ViewmodelDrawParams {
    std::string meshName = "model";
    std::string texName;
    // Viewmodel offset from camera (right, down, forward)
    float offsetX   = 0.35f;
    float offsetY   = -0.3f;
    float offsetZ   = -0.5f;
    float scale     = 0.15f;
    float rotX      = 0.0f;
    float rotY      = 0.0f;
    float rotZ      = 0.0f;
    float roughness = 0.6f;
    float metallic  = 0.4f;
};

ViewmodelDrawParams ReadViewmodelDrawParams(const WorkflowStepDefinition& step);

/// The GPU buffers and index count for one named plane mesh.
struct ViewmodelMesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer  = nullptr;
    uint32_t indexCount         = 0;
};

/// Looks up "plane_<meshName>"'s buffers/metadata; nullopt (and a
/// warning log) if any of them are missing.
std::optional<ViewmodelMesh> TryGetViewmodelMesh(
    const WorkflowContext& context, const std::string& meshName,
    const std::shared_ptr<ILogger>& logger);

/// Vertex and fragment uniforms for one viewmodel draw, built from the
/// camera/shadow state already in the context.
struct ViewmodelUniforms {
    rendering::VertexUniformData vertex;
    rendering::FragmentUniformData fragment;
};

/**
 * @brief Builds the MVP/model/normal vertex uniforms and material
 * fragment uniforms for a viewmodel draw.
 *
 * Shares BuildViewmodelMatrix with spotlight.update so a light attached
 * to this model starts exactly where the model is drawn.
 */
ViewmodelUniforms BuildViewmodelUniforms(const WorkflowContext& context,
                                         const ViewmodelDrawParams& params);

/**
 * @brief Binds the fragment sampler(s) for a viewmodel draw: the named
 * texture (falling back to the floor texture, then to nothing), plus
 * the shadow depth texture/sampler if both are present.
 */
void BindViewmodelTexture(const WorkflowContext& context,
                          SDL_GPURenderPass* pass, const std::string& texName);

}  // namespace sdl3cpp::services::impl
