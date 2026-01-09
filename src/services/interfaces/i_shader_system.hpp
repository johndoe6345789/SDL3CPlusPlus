#pragma once

#include "graphics_types.hpp"
#include "shader_system_types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Interface for shader system implementations.
 */
class IShaderSystem {
public:
    virtual ~IShaderSystem() = default;

    /**
     * @brief Unique system identifier (e.g., "materialx", "glsl").
     */
    virtual std::string GetId() const = 0;

    /**
     * @brief Build a shader map for the active system.
     */
    virtual std::unordered_map<std::string, ShaderPaths> BuildShaderMap() = 0;

    /**
     * @brief Get reflection metadata for a shader key (if supported).
     */
    virtual ShaderReflection GetReflection(const std::string& shaderKey) const = 0;

    /**
     * @brief Get default textures for a shader key (if supported).
     */
    virtual std::vector<ShaderPaths::TextureBinding> GetDefaultTextures(const std::string& shaderKey) const = 0;
};

}  // namespace sdl3cpp::services
