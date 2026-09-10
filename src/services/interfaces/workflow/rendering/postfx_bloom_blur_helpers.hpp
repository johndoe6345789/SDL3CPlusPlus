#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws one separable-blur full-screen-triangle pass: reads
 * `srcTex`, writes `dstTex`, blurring along (`dirX`, `dirY`) texel steps.
 *
 * @return false (having done nothing further) if the render pass couldn't
 * begin.
 */
bool DrawBloomBlurPass(SDL_GPUCommandBuffer* cmd,
                       SDL_GPUGraphicsPipeline* pipeline,
                       SDL_GPUTexture* srcTex, SDL_GPUTexture* dstTex,
                       SDL_GPUSampler* sampler, float dirX, float dirY);

}  // namespace sdl3cpp::services::impl
