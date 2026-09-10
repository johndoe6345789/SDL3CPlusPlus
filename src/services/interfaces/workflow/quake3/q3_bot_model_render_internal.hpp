#pragma once

/// Internal helpers shared by the q3_bot_model_render_*.cpp files that
/// together implement DrawBotModelChain() from q3_bot_model_render.hpp.
/// Not part of the public workflow-step API.

#include "services/interfaces/workflow/quake3/q3_bot_model_render.hpp"

namespace sdl3cpp::services::impl::bot_model_detail {

// Build a column-major glm matrix from a tag stored in engine Y-up coords.
// An MD3 tag stores axis[i] as the child's basis vectors already
// expressed in the parent's space, so ioq3 attaches with
// VectorMA(origin, lerped.origin[i], parent->axis[i], origin)
// (cg_ents.c CG_PositionRotatedEntityOnTag): the i-th axis scales the
// i-th component, which makes each axis a column. Transposing here
// applies the inverse rotation, which twists the torso and head off the
// legs instead of following them.
glm::mat4 TagMatrix(const nlohmann::json& tag);

// Looks up a named attachment tag for `prefix`'s given frame, returning
// identity if the MD3/frame/tag is absent.
glm::mat4 GetBotModelTagMatrix(const std::string& prefix, int frame,
                               const std::string& tagName,
                               WorkflowContext& context);

// Draws all surfaces of one MD3 model part at world transform `modelMat`.
void DrawBotModelPart(const std::string& prefix, int frame,
                      const glm::mat4& modelMat, const glm::mat4& view,
                      const glm::mat4& proj, const glm::vec3& camPos,
                      const glm::mat4& shadowVP,
                      const rendering::FragmentUniformData& fu,
                      SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                      SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                      WorkflowContext& context);

}  // namespace sdl3cpp::services::impl::bot_model_detail
