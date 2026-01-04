#pragma once

#include "graphics_types.hpp"
#include <string>
#include <unordered_map>

namespace sdl3cpp::services {

/**
 * @brief Script-facing shader lookup service interface.
 */
class IShaderScriptService {
public:
    virtual ~IShaderScriptService() = default;

    virtual std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap() = 0;
};

}  // namespace sdl3cpp::services
