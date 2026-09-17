#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"

#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief Builds the shader map from `shaderRegistry`'s active system,
 *        writing before/after debug markers under `test_outputs/` and
 *        logging the result to `logger`.
 *
 * Assumes `shaderRegistry` is non-null. Propagates whatever
 * `BuildShaderMap()` throws.
 */
std::unordered_map<std::string, ShaderPaths> BuildShaderMapWithMarkers(
    const std::shared_ptr<IShaderSystemRegistry>& shaderRegistry,
    const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
