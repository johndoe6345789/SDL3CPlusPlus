#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Graphics service configuration.
 */
struct GraphicsConfig {
    uint32_t preferredFormat = 0;  // Backend-specific format
};

/**
 * @brief Shader file paths for a shader program.
 */
struct ShaderPaths {
    std::string vertex;
    std::string vertexSource;
    std::string fragment;
    std::string fragmentSource;
    std::string geometry;
    std::string geometrySource;
    std::string tessControl;
    std::string tessControlSource;
    std::string tessEval;
    std::string tessEvalSource;
    std::string compute;
    std::string computeSource;
    struct TextureBinding {
        std::string uniformName;
        std::string path;
    };
    std::vector<TextureBinding> textures{};
    bool disableCulling = false;
    bool disableDepthTest = false;
};

/**
 * @brief View state used for per-frame uniforms.
 */
struct ViewState {
    std::array<float, 16> view{};
    std::array<float, 16> proj{};
    std::array<float, 16> viewProj{};
    std::array<float, 3> cameraPosition{};
};

/**
 * @brief Render command for a single draw call.
 */
struct RenderCommand {
    uint32_t indexOffset;
    uint32_t indexCount;
    int32_t vertexOffset;
    std::vector<std::string> shaderKeys;
    std::array<float, 16> modelMatrix;
};

} // namespace sdl3cpp::services
