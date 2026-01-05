#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services {

/**
 * @brief Input bindings for game and UI actions.
 */
struct InputBindings {
    std::string moveForwardKey = "W";
    std::string moveBackKey = "S";
    std::string moveLeftKey = "A";
    std::string moveRightKey = "D";
    std::string musicToggleKey = "M";
    std::string musicToggleGamepadButton = "start";
    std::string gamepadMoveXAxis = "leftx";
    std::string gamepadMoveYAxis = "lefty";
    std::string gamepadLookXAxis = "rightx";
    std::string gamepadLookYAxis = "righty";
    std::string gamepadDpadUpButton = "dpad_up";
    std::string gamepadDpadDownButton = "dpad_down";
    std::string gamepadDpadLeftButton = "dpad_left";
    std::string gamepadDpadRightButton = "dpad_right";
    std::unordered_map<std::string, std::string> gamepadButtonActions = {
        {"south", "gamepad_a"},
        {"east", "gamepad_b"},
        {"west", "gamepad_x"},
        {"north", "gamepad_y"},
        {"left_shoulder", "gamepad_lb"},
        {"right_shoulder", "gamepad_rb"},
        {"left_stick", "gamepad_ls"},
        {"right_stick", "gamepad_rs"},
        {"back", "gamepad_back"},
        {"start", "gamepad_start"}
    };
    std::unordered_map<std::string, std::string> gamepadAxisActions = {
        {"left_trigger", "gamepad_lt"},
        {"right_trigger", "gamepad_rt"}
    };
    float gamepadAxisActionThreshold = 0.5f;
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
    InputBindings inputBindings{};
};

}  // namespace sdl3cpp::services
