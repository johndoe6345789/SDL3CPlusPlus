#pragma once

#include "graphics_types.hpp"

#include <string>
#include <unordered_map>

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
     * @brief Resolve the active shader system id.
     */
    virtual std::string GetActiveSystemId() const = 0;
};

}  // namespace sdl3cpp::services
