#pragma once

#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/i_logger.hpp"

#include <rapidjson/document.h>
#include <string>
#include <vector>
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Migrates workflow definitions between versions
 * Supports: v2.0.0 -> v2.1.0 -> v2.2.0
 */
class WorkflowVersionMigrator {
public:
    explicit WorkflowVersionMigrator(std::shared_ptr<ILogger> logger = nullptr);

    /**
     * Detects workflow version from JSON document
     */
    std::string DetectVersion(const rapidjson::Document& document) const;

    /**
     * Migrates workflow to target version (v2.2.0)
     * Returns migrated document and list of migration warnings
     */
    std::pair<rapidjson::Document, std::vector<std::string>> Migrate(
        const rapidjson::Document& document,
        const std::string& targetVersion = "2.2.0") const;

    /**
     * Checks if version is supported for loading
     */
    bool IsSupportedVersion(const std::string& version) const;

    /**
     * Gets version compatibility matrix
     */
    struct VersionInfo {
        std::string version;
        bool canLoadNewer;      // Can this version load workflows from newer versions?
        bool canLoadOlder;      // Can this version load workflows from older versions?
        std::vector<std::string> supportedOlderVersions;
    };

    VersionInfo GetVersionInfo(const std::string& version) const;

private:
    std::shared_ptr<ILogger> logger_;

    // Migration methods
    rapidjson::Document MigrateV2_0_to_V2_1(const rapidjson::Document& doc) const;
    rapidjson::Document MigrateV2_1_to_V2_2(const rapidjson::Document& doc) const;

    // Validation
    bool ValidateV2_0_Structure(const rapidjson::Document& doc) const;
    bool ValidateV2_1_Structure(const rapidjson::Document& doc) const;
    bool ValidateV2_2_Structure(const rapidjson::Document& doc) const;

    // Helpers
    std::string NormalizeVersion(const std::string& version) const;
};

}  // namespace sdl3cpp::services::impl
