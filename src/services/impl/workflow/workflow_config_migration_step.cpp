#include "services/interfaces/workflow/workflow_config_migration_step.hpp"

#include "services/interfaces/config/json_config_migration_service.hpp"
#include "services/interfaces/config/json_config_schema_version.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowConfigMigrationStep::WorkflowConfigMigrationStep(std::shared_ptr<ILogger> logger,
                                                         std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

std::string WorkflowConfigMigrationStep::GetPluginId() const {
    return "config.migrate";
}

void WorkflowConfigMigrationStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string documentKey = resolver.GetRequiredInputKey(step, "document");
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");
    const std::string versionKey = resolver.GetRequiredInputKey(step, "version");
    const std::string outputDocumentKey = resolver.GetRequiredOutputKey(step, "document");
    const std::string outputVersionKey = resolver.GetRequiredOutputKey(step, "version");

    const auto* documentHandle = context.TryGet<std::shared_ptr<rapidjson::Document>>(documentKey);
    if (!documentHandle || !(*documentHandle)) {
        throw std::runtime_error("Workflow config.migrate missing document input '" + documentKey + "'");
    }
    std::shared_ptr<rapidjson::Document> document = *documentHandle;

    const auto* versionHandle = context.TryGet<std::optional<int>>(versionKey);
    if (!versionHandle) {
        throw std::runtime_error("Workflow config.migrate missing version input '" + versionKey + "'");
    }
    std::optional<int> version = *versionHandle;

    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("Workflow config.migrate missing path input '" + pathKey + "'");
    }

    if (!version) {
        if (logger_) {
            logger_->Trace("WorkflowConfigMigrationStep", "Execute",
                           "configPath=" + pathValue.string(),
                           "No schema version provided; skipping migration");
        }
    } else if (*version == json_config::kRuntimeConfigSchemaVersion) {
        if (logger_) {
            logger_->Trace("WorkflowConfigMigrationStep", "Execute",
                           "version=" + std::to_string(*version),
                           "Schema version matches runtime; skipping migration");
        }
    } else {
        if (logger_) {
            logger_->Info("WorkflowConfigMigrationStep: Migrating config from version " +
                          std::to_string(*version) + " to " +
                          std::to_string(json_config::kRuntimeConfigSchemaVersion));
        }
        json_config::JsonConfigMigrationService migrationService(logger_, probeService_);
        const bool migrated = migrationService.Apply(*document,
                                                     *version,
                                                     json_config::kRuntimeConfigSchemaVersion,
                                                     pathValue);
        if (!migrated) {
            throw std::runtime_error("Unsupported schema version " + std::to_string(*version) +
                                     " in " + pathValue.string() +
                                     "; expected " + std::to_string(json_config::kRuntimeConfigSchemaVersion) +
                                     " (see config/schema/MIGRATIONS.md)");
        }
        version = json_config::kRuntimeConfigSchemaVersion;
    }

    context.Set(outputDocumentKey, document);
    context.Set(outputVersionKey, version);
}

}  // namespace sdl3cpp::services::impl
