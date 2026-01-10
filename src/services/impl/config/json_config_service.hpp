#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/config_types.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

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
     * @param argv0 First command-line argument (for resolving default project root)
     * @param probeService Probe service for diagnostics (optional)
     */
    JsonConfigService(std::shared_ptr<ILogger> logger,
                      const char* argv0,
                      std::shared_ptr<IProbeService> probeService = nullptr);

    /**
     * @brief Construct by loading configuration from JSON.
     *
     * @param logger Logger service for logging
     * @param configPath Path to JSON configuration file
     * @param dumpConfig Whether to print loaded config to stdout
     * @param probeService Probe service for diagnostics (optional)
     * @throws std::runtime_error if config file cannot be loaded or is invalid
     */
    JsonConfigService(std::shared_ptr<ILogger> logger,
                      const std::filesystem::path& configPath,
                      bool dumpConfig,
                      std::shared_ptr<IProbeService> probeService = nullptr);

    /**
     * @brief Construct with explicit configuration.
     *
     * @param logger Logger service for logging
     * @param config Runtime configuration to use
     * @param probeService Probe service for diagnostics (optional)
     */
    JsonConfigService(std::shared_ptr<ILogger> logger,
                      const RuntimeConfig& config,
                      std::shared_ptr<IProbeService> probeService = nullptr);

    // IConfigService interface implementation
    uint32_t GetWindowWidth() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetWindowWidth");
        }
        return config_.width;
    }
    uint32_t GetWindowHeight() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetWindowHeight");
        }
        return config_.height;
    }
    std::filesystem::path GetProjectRoot() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetProjectRoot");
        }
        return config_.projectRoot;
    }
    std::string GetWindowTitle() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetWindowTitle");
        }
        return config_.windowTitle;
    }
    const InputBindings& GetInputBindings() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetInputBindings");
        }
        return config_.inputBindings;
    }
    const MouseGrabConfig& GetMouseGrabConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetMouseGrabConfig");
        }
        return config_.mouseGrab;
    }
    const BgfxConfig& GetBgfxConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetBgfxConfig");
        }
        return config_.bgfx;
    }
    const MaterialXConfig& GetMaterialXConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetMaterialXConfig");
        }
        return config_.materialX;
    }
    const std::vector<MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetMaterialXMaterialConfigs");
        }
        return config_.materialXMaterials;
    }
    const GuiFontConfig& GetGuiFontConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetGuiFontConfig");
        }
        return config_.guiFont;
    }
    const RenderBudgetConfig& GetRenderBudgetConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetRenderBudgetConfig");
        }
        return config_.budgets;
    }
    const CrashRecoveryConfig& GetCrashRecoveryConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetCrashRecoveryConfig");
        }
        return config_.crashRecovery;
    }
    const ValidationTourConfig& GetValidationTourConfig() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetValidationTourConfig");
        }
        return config_.validationTour;
    }
    const std::string& GetConfigJson() const override {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetConfigJson");
        }
        return configJson_;
    }

    /**
     * @brief Get the full runtime configuration.
     *
     * @return Reference to the config structure
     */
    const RuntimeConfig& GetConfig() const {
        if (logger_) {
            logger_->Trace("JsonConfigService", "GetConfig");
        }
        return config_;
    }

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
    std::string configJson_;
    RuntimeConfig config_;

    // Helper methods moved from main.cpp
    std::filesystem::path ResolveProjectRoot(const char* argv0);
    static RuntimeConfig LoadFromJson(std::shared_ptr<ILogger> logger,
                                      std::shared_ptr<IProbeService> probeService,
                                      const std::filesystem::path& configPath,
                                      bool dumpConfig,
                                      std::string* configJson);
    static std::string BuildConfigJson(const RuntimeConfig& config, const std::filesystem::path& configPath);
};

}  // namespace sdl3cpp::services::impl
