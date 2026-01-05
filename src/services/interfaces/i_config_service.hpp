#pragma once

#include "config_types.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Configuration service interface.
 *
 * Provides access to application configuration loaded from JSON or defaults.
 * Similar to Spring's @ConfigurationProperties.
 */
class IConfigService {
public:
    virtual ~IConfigService() = default;

    /**
     * @brief Get the configured window width.
     * @return Window width in pixels
     */
    virtual uint32_t GetWindowWidth() const = 0;

    /**
     * @brief Get the configured window height.
     * @return Window height in pixels
     */
    virtual uint32_t GetWindowHeight() const = 0;

    /**
     * @brief Get the path to the Lua script to execute.
     * @return Path to the script file
     */
    virtual std::filesystem::path GetScriptPath() const = 0;

    /**
     * @brief Check if Lua debug mode is enabled.
     * @return true if debug mode is enabled, false otherwise
     */
    virtual bool IsLuaDebugEnabled() const = 0;

    /**
     * @brief Get the window title.
     * @return Window title string
     */
    virtual std::string GetWindowTitle() const = 0;

    /**
     * @brief Get required Vulkan device extensions.
     * @return List of extension names
     */
    virtual std::vector<const char*> GetDeviceExtensions() const = 0;

    /**
     * @brief Get configured input bindings.
     * @return Input bindings structure
     */
    virtual const InputBindings& GetInputBindings() const = 0;
};

}  // namespace sdl3cpp::services
