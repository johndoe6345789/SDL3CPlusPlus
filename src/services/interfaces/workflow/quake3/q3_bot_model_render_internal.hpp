#pragma once

/// Internal helpers shared by the q3_bot_model_render_*.cpp files that
/// together implement DrawBotModelChain() from q3_bot_model_render.hpp.
/// Not part of the public workflow-step API.

#include "services/interfaces/workflow/quake3/q3_bot_model_render.hpp"

namespace sdl3cpp::services::impl::bot_model_detail {

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
