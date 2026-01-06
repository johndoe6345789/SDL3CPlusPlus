#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Input bindings for game and UI actions.
 */
struct InputBindings {
    std::string moveForwardKey = "W";
    std::string moveBackKey = "S";
    std::string moveLeftKey = "A";
    std::string moveRightKey = "D";
    std::string flyUpKey = "Q";
    std::string flyDownKey = "Z";
    std::string jumpKey = "Space";
    std::string noclipToggleKey = "N";
    std::string musicToggleKey = "M";
    std::string musicToggleGamepadButton = "start";
    std::string gamepadMoveXAxis = "leftx";
    std::string gamepadMoveYAxis = "lefty";
    std::string gamepadLookXAxis = "rightx";
    std::string gamepadLookYAxis = "righty";
    std::string gamepadDpadUpButton = "dpup";
    std::string gamepadDpadDownButton = "dpdown";
    std::string gamepadDpadLeftButton = "dpleft";
    std::string gamepadDpadRightButton = "dpright";
    std::unordered_map<std::string, std::string> gamepadButtonActions = {
        {"a", "gamepad_a"},
        {"b", "gamepad_b"},
        {"x", "gamepad_x"},
        {"y", "gamepad_y"},
        {"leftshoulder", "gamepad_lb"},
        {"rightshoulder", "gamepad_rb"},
        {"leftstick", "gamepad_ls"},
        {"rightstick", "gamepad_rs"},
        {"back", "gamepad_back"},
        {"start", "gamepad_start"}
    };
    std::unordered_map<std::string, std::string> gamepadAxisActions = {
        {"lefttrigger", "gamepad_lt"},
        {"righttrigger", "gamepad_rt"}
    };
    float gamepadAxisActionThreshold = 0.5f;
};

/**
 * @brief Mouse grabbing behavior configuration.
 */
struct MouseGrabConfig {
    bool enabled = false;
    bool grabOnClick = true;
    bool releaseOnEscape = true;
    bool startGrabbed = false;
    bool hideCursor = true;
    bool relativeMode = true;
    std::string grabMouseButton = "left";
    std::string releaseKey = "escape";
};

/**
 * @brief Atmospherics and lighting configuration.
 */
struct AtmosphericsConfig {
    float ambientStrength = 0.01f;
    float fogDensity = 0.003f;
    std::array<float, 3> fogColor = {0.05f, 0.05f, 0.08f};
    std::array<float, 3> skyColor = {0.1f, 0.1f, 0.15f};
    float gamma = 2.2f;
    float exposure = 1.0f;
    bool enableToneMapping = true;
    bool enableShadows = true;
    bool enableSSGI = true;
    bool enableVolumetricLighting = true;
    float pbrRoughness = 0.3f;
    float pbrMetallic = 0.1f;
};

/**
 * @brief Lua-defined render graph configuration.
 */
struct RenderGraphConfig {
    bool enabled = false;
    std::string functionName = "get_render_graph";
};

enum class GraphicsBackendType {
    Vulkan,
    Bgfx
};

struct GraphicsBackendConfig {
    GraphicsBackendType backend = GraphicsBackendType::Vulkan;
    std::string bgfxRenderer = "vulkan";
};

struct MaterialXConfig {
    bool enabled = false;
    std::filesystem::path documentPath;
    std::string shaderKey = "materialx";
    std::string materialName;
    std::filesystem::path libraryPath;
    std::vector<std::string> libraryFolders = {
        "stdlib",
        "pbrlib",
        "lights",
        "bxdf",
        "cmlib",
        "nprlib",
        "targets"
    };
    bool useConstantColor = false;
    std::array<float, 3> constantColor = {1.0f, 1.0f, 1.0f};
};

struct GuiFontConfig {
    bool useFreeType = false;
    std::filesystem::path fontPath;
    float fontSize = 18.0f;
};

/**
 * @brief Runtime configuration values used across services.
 */
struct RuntimeConfig {
    uint32_t width = 1024;
    uint32_t height = 768;
    std::filesystem::path scriptPath;
    bool luaDebug = false;
    std::string windowTitle = "SDL3 Vulkan Demo";
    MouseGrabConfig mouseGrab{};
    InputBindings inputBindings{};
    AtmosphericsConfig atmospherics{};
    RenderGraphConfig renderGraph{};
    GraphicsBackendConfig graphicsBackend{};
    MaterialXConfig materialX{};
    GuiFontConfig guiFont{};
    float guiOpacity = 1.0f;
};

}  // namespace sdl3cpp::services
