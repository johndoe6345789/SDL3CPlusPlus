#pragma once

#include "services/interfaces/workflow/rendering/box_face_builder.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>

namespace sdl3cpp::services::impl {

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
                  const std::array<BoxFace, 6>& faces, const glm::vec3& center,
                  const glm::mat4& bodyRotation, const glm::mat4& view,
                  const glm::mat4& proj, const glm::vec3& camPos,
                  const glm::mat4& shadowVP,
                  const rendering::FragmentUniformData& fu,
                  uint32_t indexCount);

}  // namespace sdl3cpp::services::impl
