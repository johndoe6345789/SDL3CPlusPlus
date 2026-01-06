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

    /**
     * @brief Get configured mouse grab settings.
     * @return Mouse grab configuration
     */
    virtual const MouseGrabConfig& GetMouseGrabConfig() const = 0;

    /**
     * @brief Get render graph settings.
     * @return Render graph configuration
     */
    virtual const RenderGraphConfig& GetRenderGraphConfig() const = 0;

    /**
     * @brief Get graphics backend settings.
     * @return Graphics backend configuration
     */
    virtual const GraphicsBackendConfig& GetGraphicsBackendConfig() const = 0;

    /**
     * @brief Get MaterialX settings.
     * @return MaterialX configuration
     */
    virtual const MaterialXConfig& GetMaterialXConfig() const = 0;

    /**
     * @brief Get GUI font settings.
     * @return GUI font configuration
     */
    virtual const GuiFontConfig& GetGuiFontConfig() const = 0;

    /**
     * @brief Get the full JSON configuration as a string.
     *
     * @return JSON string (may be empty if unavailable)
     */
    virtual const std::string& GetConfigJson() const = 0;
};

}  // namespace sdl3cpp::services
