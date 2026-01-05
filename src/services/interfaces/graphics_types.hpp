#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace sdl3cpp::services {

/**
 * @brief Graphics service configuration.
 */
struct GraphicsConfig {
    std::vector<const char*> deviceExtensions;
    uint32_t preferredFormat = 0;  // Backend-specific format
    bool enableValidationLayers = false;
};

/**
 * @brief Shader file paths for a shader program.
 */
struct ShaderPaths {
    std::string vertex;
    std::string fragment;
    std::string geometry;
    std::string tessControl;
    std::string tessEval;
    std::string compute;
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
