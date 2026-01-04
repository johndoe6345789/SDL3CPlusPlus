#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Runtime configuration values used across services.
 */
struct RuntimeConfig {
    uint32_t width = 1024;
    uint32_t height = 768;
    std::filesystem::path scriptPath;
    bool luaDebug = false;
    std::string windowTitle = "SDL3 Vulkan Demo";
};

}  // namespace sdl3cpp::services
