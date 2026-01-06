#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

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

/**
 * @brief Render graph value used for Lua-defined settings.
 */
struct RenderGraphValue {
    enum class Type {
        Number,
        Boolean,
        String,
        Array
    };

    Type type = Type::Number;
    double number = 0.0;
    bool boolean = false;
    std::string string;
    std::vector<double> array;
};

/**
 * @brief Render graph resource definition loaded from Lua.
 */
struct RenderGraphResource {
    std::string name;
    std::string type;
    std::string format;
    std::string size;
    std::array<uint32_t, 2> explicitSize{};
    bool hasExplicitSize = false;
    uint32_t layers = 1;
    uint32_t mips = 1;
};

/**
 * @brief Render graph pass definition loaded from Lua.
 */
struct RenderGraphPass {
    std::string name;
    std::string kind;
    std::string shader;
    std::string output;
    std::unordered_map<std::string, std::string> inputs;
    std::unordered_map<std::string, std::string> outputs;
    std::unordered_map<std::string, RenderGraphValue> settings;
};

/**
 * @brief Render graph definition loaded from Lua.
 */
struct RenderGraphDefinition {
    std::vector<RenderGraphResource> resources;
    std::vector<RenderGraphPass> passes;
};

} // namespace sdl3cpp::services
