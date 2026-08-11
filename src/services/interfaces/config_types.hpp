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

struct GpuConfig {
    std::string renderer = "auto";  // Auto-select: Metal on macOS, Vulkan on Linux, D3D on Windows
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

struct MaterialXMaterialConfig {
    bool enabled = true;
    std::filesystem::path documentPath;
    std::string shaderKey;
    std::string materialName;
    bool useConstantColor = false;
    std::array<float, 3> constantColor = {1.0f, 1.0f, 1.0f};
};

struct GuiFontConfig {
    bool useFreeType = true;
    std::filesystem::path fontPath;
    float fontSize = 18.0f;
};

/**
 * @brief Resource budget and rendering limits.
 */
struct RenderBudgetConfig {
    size_t vramMB = 512;
    uint32_t maxTextureDim = 0;
    size_t guiTextCacheEntries = 256;
    size_t guiSvgCacheEntries = 64;
};

/**
 * @brief Crash recovery tuning parameters.
 */
struct CrashRecoveryConfig {
    uint32_t heartbeatTimeoutMs = 5000;
    uint32_t heartbeatPollIntervalMs = 200;
    size_t memoryLimitMB = 1024;
    double gpuHangFrameTimeMultiplier = 10.0;
    size_t maxConsecutiveGpuTimeouts = 5;
    size_t maxFileFormatErrors = 2;
    size_t maxMemoryWarnings = 3;
};

/**
 * @brief Camera configuration for validation checkpoints.
 */
struct ValidationCameraConfig {
    std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> lookAt = {0.0f, 0.0f, -1.0f};
    std::array<float, 3> up = {0.0f, 1.0f, 0.0f};
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

/**
 * @brief Expected output for a validation checkpoint.
 */
struct ValidationExpectedConfig {
    bool enabled = false;
    std::filesystem::path imagePath;
    float tolerance = 0.01f;
    size_t maxDiffPixels = 0;
};

/**
 * @brief Sample point expectation for validation checks.
 */
struct ValidationSamplePointConfig {
    float x = 0.5f;
    float y = 0.5f;
    std::array<float, 3> color = {0.0f, 0.0f, 0.0f};
    float tolerance = 0.1f;
};

/**
 * @brief Validation check definitions that do not require a baseline image.
 */
struct ValidationCheckConfig {
    std::string type;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float threshold = 0.05f;
    float tolerance = 0.1f;
    std::array<float, 3> color = {0.0f, 0.0f, 0.0f};
    std::vector<ValidationSamplePointConfig> points{};
};

/**
 * @brief A single validation checkpoint definition.
 */
struct ValidationCheckpointConfig {
    std::string id;
    ValidationCameraConfig camera{};
    ValidationExpectedConfig expected{};
    std::vector<ValidationCheckConfig> checks{};
};

/**
 * @brief Config-driven validation tour at startup.
 */
struct ValidationTourConfig {
    bool enabled = false;
    bool failOnMismatch = true;
    uint32_t warmupFrames = 2;
    uint32_t captureFrames = 1;
    std::filesystem::path outputDir = "artifacts/validation";
    std::vector<ValidationCheckpointConfig> checkpoints{};
};

/**
 * @brief Runtime configuration values used across services.
 */
struct RuntimeConfig {
    uint32_t width = 1024;
    uint32_t height = 768;
    std::filesystem::path projectRoot;
    std::string windowTitle = "SDL3 GPU Demo";
    MouseGrabConfig mouseGrab{};
    InputBindings inputBindings{};
    AtmosphericsConfig atmospherics{};
    GpuConfig gpu{};
    MaterialXConfig materialX{};
    std::vector<MaterialXMaterialConfig> materialXMaterials{};
    RenderBudgetConfig budgets{};
    GuiFontConfig guiFont{};
    float guiOpacity = 1.0f;
    CrashRecoveryConfig crashRecovery{};
    ValidationTourConfig validationTour{};
};

}  // namespace sdl3cpp::services
