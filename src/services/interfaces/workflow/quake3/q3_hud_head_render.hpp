#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// Q3A-style idle head sway state (mirrors cg.headStart/EndYaw/Pitch/Time).
struct HeadSwayState {
    float swayStartYaw   = glm::radians(180.f);
    float swayStartPitch = 0.f;
    float swayEndYaw     = glm::radians(180.f);
    float swayEndPitch   = 0.f;
    uint64_t swayStartMs = 0;
    uint64_t swayEndMs   = 0;
};

/// One yaw/pitch pair, in radians.
struct HeadAngles {
    float yaw   = 0.f;
    float pitch = 0.f;
};

/**
 * @brief Advances the idle head-sway animation and returns the current pose.
 *
 * Mirrors ioq3 CG_DrawStatusBarHead: every 100-2100 ms picks a new target
 * yaw (180 deg +/- 20 deg) and pitch (+/- 5 deg), smoothstep-interpolating
 * toward it. Mutates `state` in place.
 */
HeadAngles UpdateHeadSway(HeadSwayState& state, uint64_t nowMs);

/**
 * @brief Builds the head-at-origin MVP for the given sway pose.
 *
 * The camera orbits the head at `camDist` using `angles`; Q3A places its
 * origin at (len/tan(15 deg), 0, 0) in model space, which this
 * approximates. Uses a 30 deg FOV to match Q3A's status-bar head.
 */
glm::mat4 BuildHeadPortraitMvp(const HeadAngles& angles, float camDist);

/// Soft front-right key light + ambient, matching the original portrait.
rendering::FragmentUniformData DefaultHeadPortraitLighting();

/// Color + depth render targets used to render the head portrait offscreen.
struct HeadRenderTargets {
    SDL_GPUTexture* color = nullptr;
    SDL_GPUTexture* depth = nullptr;
    bool ready            = false;
};

/**
 * @brief Creates the `size` x `size` color/depth render targets.
 *
 * The color target's format matches the swapchain's so the caller's
 * textured pipeline (compiled for the swapchain) can render into it
 * without a format-mismatch error.
 */
HeadRenderTargets CreateHeadRenderTargets(SDL_GPUDevice* device,
                                          SDL_Window* window, int size);

/**
 * @brief Draws all surfaces of a named MD3 into an already-bound pass.
 *
 * `prefix` selects the MD3 previously uploaded to context under
 * `q3.md3.<prefix>_*` keys (see WorkflowQ3Md3UploadSurfacesStep).
 */
void DrawHeadMd3(const std::string& prefix, const glm::mat4& mvp,
                 const glm::mat4& model,
                 const rendering::FragmentUniformData& fu,
                 SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                 WorkflowContext& context);

/**
 * @brief Opens an offscreen pass on `targets`, draws the head, and ends it.
 *
 * @param size Render target width/height (square) used for the viewport.
 * @return false if the render pass could not be opened, in which case the
 *         caller should treat the frame as having no portrait to show.
 */
bool RenderHeadPortraitPass(SDL_GPUCommandBuffer* cmd,
                            const HeadRenderTargets& targets,
                            SDL_GPUGraphicsPipeline* pipeline,
                            const glm::mat4& mvp, const glm::mat4& model,
                            const rendering::FragmentUniformData& fu,
                            WorkflowContext& context, int size);

}  // namespace sdl3cpp::services::impl
