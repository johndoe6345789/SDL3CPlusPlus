#pragma once

#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Runtime configuration structure.
 */
struct RuntimeConfig {
    uint32_t width = 1024;
    uint32_t height = 768;
    std::filesystem::path scriptPath;
    bool luaDebug = false;
    std::string windowTitle = "SDL3 Vulkan Demo";
};

/**
 * @brief JSON-based configuration service implementation.
 *
 * Loads application configuration from JSON files or provides defaults.
 * Implements the IConfigService interface.
 */
class JsonConfigService : public IConfigService {
public:
    /**
     * @brief Construct with default configuration.
     *
     * @param logger Logger service for logging
     * @param argv0 First command-line argument (for finding default script path)
     */
    JsonConfigService(std::shared_ptr<ILogger> logger, const char* argv0);

    /**
     * @brief Construct by loading configuration from JSON.
     *
     * @param logger Logger service for logging
     * @param configPath Path to JSON configuration file
     * @param dumpConfig Whether to print loaded config to stdout
     * @throws std::runtime_error if config file cannot be loaded or is invalid
     */
    JsonConfigService(std::shared_ptr<ILogger> logger, const std::filesystem::path& configPath, bool dumpConfig);

    /**
     * @brief Construct with explicit configuration.
     *
     * @param logger Logger service for logging
     * @param config Runtime configuration to use
     */
    JsonConfigService(std::shared_ptr<ILogger> logger, const RuntimeConfig& config);

    // IConfigService interface implementation
    uint32_t GetWindowWidth() const override { return config_.width; }
    uint32_t GetWindowHeight() const override { return config_.height; }
    std::filesystem::path GetScriptPath() const override { return config_.scriptPath; }
    bool IsLuaDebugEnabled() const override { return config_.luaDebug; }
    std::string GetWindowTitle() const override { return config_.windowTitle; }
    std::vector<const char*> GetDeviceExtensions() const override;

    /**
     * @brief Get the full runtime configuration.
     *
     * @return Reference to the config structure
     */
    const RuntimeConfig& GetConfig() const { return config_; }

private:
    std::shared_ptr<ILogger> logger_;
    RuntimeConfig config_;

    // Helper methods moved from main.cpp
    static std::filesystem::path FindScriptPath(const char* argv0);
    static RuntimeConfig LoadFromJson(std::shared_ptr<ILogger> logger, const std::filesystem::path& configPath, bool dumpConfig);
};

}  // namespace sdl3cpp::services::impl
