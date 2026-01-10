#include "json_config_migration_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"

#include <string>

namespace sdl3cpp::services::impl::json_config {

JsonConfigMigrationService::JsonConfigMigrationService(std::shared_ptr<ILogger> logger,
                                                       std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

bool JsonConfigMigrationService::Apply(rapidjson::Document& document,
                                       int fromVersion,
                                       int toVersion,
                                       const std::filesystem::path& configPath) const {
    (void)document;
    if (fromVersion == toVersion) {
        return true;
    }
    if (logger_) {
        logger_->Warn("JsonConfigService::ApplyMigrations: No migration path from v" +
                      std::to_string(fromVersion) + " to v" + std::to_string(toVersion) +
                      " for " + configPath.string());
    }
    if (probeService_) {
        ProbeReport report{};
        report.severity = ProbeSeverity::Error;
        report.code = "CONFIG_MIGRATION_MISSING";
        report.jsonPath = "";
        report.message = "No migration path from v" + std::to_string(fromVersion) +
                         " to v" + std::to_string(toVersion) +
                         " (see config/schema/MIGRATIONS.md)";
        probeService_->Report(report);
    }
    return false;
}

}  // namespace sdl3cpp::services::impl::json_config
