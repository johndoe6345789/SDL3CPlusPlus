#pragma once

#include "services/interfaces/workflow/graphics/shader_compile_params.hpp"
#include "services/interfaces/workflow/graphics/shader_format_detection.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Creates the compiled SDL_GPUShader for one shader stage.
 *
 * `shaderData` must already have been passed through
 * PrepareShaderBinaryForFormat. Excludes MSL's appended null terminator
 * from `code_size`; SPIR-V uses `shaderData.size()` as-is. Throws
 * std::runtime_error (prefixed "graphics.gpu.shader.compile: ...") if
 * shader creation fails.
 */
SDL_GPUShader* CreateCompiledShader(SDL_GPUDevice* device,
                                    const ShaderFormatInfo& formatInfo,
                                    const ShaderCompileParams& params,
                                    const std::vector<uint8_t>& shaderData);

}  // namespace sdl3cpp::services::impl
