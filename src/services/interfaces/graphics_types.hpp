#pragma once

#include <string>
#include <array>

namespace sdl3cpp::services {

/**
 * @brief Shader file paths for a shader program.
 */
struct ShaderPaths {
    std::string vertex;
    std::string fragment;
};

/**
 * @brief Render command for a single draw call.
 */
struct RenderCommand {
    uint32_t indexOffset;
    uint32_t indexCount;
    int32_t vertexOffset;
    std::string shaderKey;
    std::array<float, 16> modelMatrix;
};

} // namespace sdl3cpp::services