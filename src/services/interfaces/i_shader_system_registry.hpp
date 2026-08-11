#pragma once

#include "services/interfaces/graphics_types.hpp"
#include "services/interfaces/shader_system_types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Registry that selects and executes the active shader system.
 */
class IShaderSystemRegistry {
public:
    virtual ~IShaderSystemRegistry() = default;

    /**
     * @brief Build a shader map using the active shader system.
     */
    virtual std::unordered_map<std::string, ShaderPaths> BuildShaderMap() = 0;

    /**
     * @brief Get reflection metadata for the active shader system.
     */
    virtual ShaderReflection GetReflection(const std::string& shaderKey) const = 0;

    /**
     * @brief Get default textures for the active shader system.
     */
    virtual std::vector<ShaderPaths::TextureBinding> GetDefaultTextures(
        const std::string& shaderKey) const = 0;

    /**
     * @brief Resolve the active shader system id.
     */
    virtual std::string GetActiveSystemId() const = 0;
};

}  // namespace sdl3cpp::services
