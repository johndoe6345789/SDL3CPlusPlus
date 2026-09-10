#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// Parameters for one `draw.textured_box` call, already resolved from the
/// step's JSON parameters.
struct DrawTexturedBoxParams {
    glm::vec3 pos{0.0f};
    glm::vec3 size{1.0f};
    float uvDensity = 1.0f;
    float roughness = 0.8f;
    float metallic = 0.0f;
    std::string texture = "walls_texture";
    std::string body;
};

/// One face of an axis-aligned box: its center-relative offset, outward
/// normal, orientation, and UV tiling in world units.
struct BoxFace {
    glm::vec3 offset;
    glm::vec3 normal;
    glm::mat4 rotation;
    float scaleW, scaleD;
    float uvW, uvH;
};

/// Builds the 6 faces of a `size_x` x `size_y` x `size_z` box centered on
/// the origin, tiling each face's texture at `uvDensity` repeats per unit.
std::array<BoxFace, 6> BuildBoxFaces(float sizeX, float sizeY, float sizeZ,
                                     float uvDensity);

/// Binds `texture`/`sampler` at slot 0, and — when both are non-null —
/// `shadowTex`/`shadowSamp` at slot 1; otherwise only slot 0 is bound.
void BindBoxTextures(SDL_GPURenderPass* pass, SDL_GPUTexture* texture,
                     SDL_GPUSampler* sampler, SDL_GPUTexture* shadowTex,
                     SDL_GPUSampler* shadowSamp);

/**
 * @brief Draws each of `faces` as one indexed draw call on the unit plane.
 *
 * Each face's model matrix is `translate(center) * bodyRotation *
 * translate(face.offset) * face.rotation * scale(face.scaleW, 1,
 * face.scaleD)`, applied to the caller's bound unit-plane vertex/index
 * buffers (already bound by the caller).
 */
void DrawBoxFaces(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                  const std::array<BoxFace, 6>& faces,
                  const glm::vec3& center, const glm::mat4& bodyRotation,
                  const glm::mat4& view, const glm::mat4& proj,
                  const glm::vec3& camPos, const glm::mat4& shadowVP,
                  const rendering::FragmentUniformData& fu,
                  uint32_t indexCount);

/**
 * @brief Runs the full `draw.textured_box` draw for one call.
 *
 * Looks up the unit-plane mesh, `params.texture`'s GPU texture/sampler, the
 * optional shadow map, camera/lighting state, and (when `params.body` is
 * set) that body's synced transform, then draws all 6 faces. Logs a
 * warning and returns without drawing if a required resource — the
 * render pass/command buffer/pipeline, the unit plane, or the texture —
 * is missing from `context`.
 */
void DrawTexturedBox(WorkflowContext& context, ILogger* logger,
                    const DrawTexturedBoxParams& params);

}  // namespace sdl3cpp::services::impl
