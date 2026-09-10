#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Loads the fixed-path overlay SPIR-V shaders and compiles them.
 *
 * The overlay only ships SPIR-V, so this is only meaningful on the Vulkan
 * backend; callers are expected to have already checked the active driver.
 *
 * @param device      GPU device to compile the shaders against.
 * @param outVertex   Set to the compiled vertex shader on success.
 * @param outFragment Set to the compiled fragment shader on success.
 * @return Empty string on success, otherwise the reason loading failed.
 *         Neither out param is touched on failure.
 */
const char* LoadOverlayShaderPair(SDL_GPUDevice* device,
                                  SDL_GPUShader*& outVertex,
                                  SDL_GPUShader*& outFragment);

}  // namespace sdl3cpp::services::impl
