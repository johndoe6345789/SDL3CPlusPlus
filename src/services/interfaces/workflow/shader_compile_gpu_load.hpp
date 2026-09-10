#pragma once

#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"

#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief Loads `shaderMap` to the GPU via `graphicsService`, if non-null.
 *
 * A load failure is logged as a warning and swallowed — shader.compile
 * doesn't fail the step just because the GPU load failed, since the
 * shaders are still compiled either way.
 */
void LoadShaderMapToGpu(
    const std::shared_ptr<IGraphicsService>& graphicsService,
    const std::unordered_map<std::string, ShaderPaths>& shaderMap,
    const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
