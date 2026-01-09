#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Minimal reflection snapshot for a shader system.
 */
struct ShaderReflection {
    std::vector<std::string> uniforms;
    std::vector<std::string> textures;
};

}  // namespace sdl3cpp::services
