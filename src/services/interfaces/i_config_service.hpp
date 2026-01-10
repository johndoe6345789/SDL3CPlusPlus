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
     * @brief Get the resolved project root path.
     * @return Project root path
     */
    virtual std::filesystem::path GetProjectRoot() const = 0;

    /**
     * @brief Get the window title.
     * @return Window title string
     */
    virtual std::string GetWindowTitle() const = 0;

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
     * @brief Get bgfx settings.
     * @return Bgfx configuration
     */
    virtual const BgfxConfig& GetBgfxConfig() const = 0;

    /**
     * @brief Get MaterialX settings.
     * @return MaterialX configuration
     */
    virtual const MaterialXConfig& GetMaterialXConfig() const = 0;

    /**
     * @brief Get MaterialX material overrides.
     * @return MaterialX material configurations
     */
    virtual const std::vector<MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const = 0;

    /**
     * @brief Get GUI font settings.
     * @return GUI font configuration
     */
    virtual const GuiFontConfig& GetGuiFontConfig() const = 0;

    /**
     * @brief Get rendering budgets.
     * @return Render budget configuration
     */
    virtual const RenderBudgetConfig& GetRenderBudgetConfig() const = 0;

    /**
     * @brief Get crash recovery configuration.
     * @return Crash recovery configuration
     */
    virtual const CrashRecoveryConfig& GetCrashRecoveryConfig() const = 0;

    /**
     * @brief Get validation tour configuration.
     * @return Validation tour configuration
     */
    virtual const ValidationTourConfig& GetValidationTourConfig() const = 0;

    /**
     * @brief Get the full JSON configuration as a string.
     *
     * @return JSON string (may be empty if unavailable)
     */
    virtual const std::string& GetConfigJson() const = 0;
};

}  // namespace sdl3cpp::services
