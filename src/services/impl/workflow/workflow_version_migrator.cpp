#include "services/interfaces/workflow/workflow_version_migrator.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace sdl3cpp::services::impl {

WorkflowVersionMigrator::WorkflowVersionMigrator(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowVersionMigrator", "Constructor", "Entry");
    }
}

std::string WorkflowVersionMigrator::DetectVersion(const rapidjson::Document& document) const {
    if (!document.IsObject()) {
        throw std::runtime_error("Workflow must be a JSON object");
    }

    // Check for version field
    if (document.HasMember("version")) {
        if (!document["version"].IsString()) {
            throw std::runtime_error("Workflow 'version' field must be a string");
        }
        return document["version"].GetString();
    }

    // Detect version based on structure
    // v2.0.0: has "steps" array with old format
    // v2.1.0: has "nodes" array and "connections"
    // v2.2.0: has "nodes", "connections", "type" field, "typeVersion" in nodes

    if (document.HasMember("steps") && document["steps"].IsArray()) {
        return "2.0.0";  // Old steps format
    }

    if (document.HasMember("nodes") && document["nodes"].IsArray()) {
        bool hasTypeVersions = false;
        for (const auto& node : document["nodes"].GetArray()) {
            if (node.IsObject() && node.HasMember("typeVersion")) {
                hasTypeVersions = true;
                break;
            }
        }
        return hasTypeVersions ? "2.2.0" : "2.1.0";
    }

    throw std::runtime_error("Cannot detect workflow version - unsupported format");
}

std::string WorkflowVersionMigrator::NormalizeVersion(const std::string& version) const {
    // Normalize version strings (e.g., "2.0" -> "2.0.0")
    if (version == "1.0" || version == "1.0.0") {
        return "1.0.0";
    }
    if (version == "2.0" || version == "2.0.0") {
        return "2.0.0";
    }
    if (version == "2.1" || version == "2.1.0") {
        return "2.1.0";
    }
    if (version == "2.2" || version == "2.2.0") {
        return "2.2.0";
    }
    return version;
}

bool WorkflowVersionMigrator::IsSupportedVersion(const std::string& version) const {
    std::string normalized = NormalizeVersion(version);
    return normalized == "2.0.0" || normalized == "2.1.0" || normalized == "2.2.0";
}

WorkflowVersionMigrator::VersionInfo WorkflowVersionMigrator::GetVersionInfo(
    const std::string& version) const {
    std::string normalized = NormalizeVersion(version);

    if (normalized == "2.0.0") {
        return {
            .version = "2.0.0",
            .canLoadNewer = false,
            .canLoadOlder = false,
            .supportedOlderVersions = {}
        };
    }

    if (normalized == "2.1.0") {
        return {
            .version = "2.1.0",
            .canLoadNewer = false,
            .canLoadOlder = true,
            .supportedOlderVersions = {"2.0.0"}
        };
    }

    if (normalized == "2.2.0") {
        return {
            .version = "2.2.0",
            .canLoadNewer = false,
            .canLoadOlder = true,
            .supportedOlderVersions = {"2.0.0", "2.1.0"}
        };
    }

    throw std::runtime_error("Unknown version: " + version);
}

bool WorkflowVersionMigrator::ValidateV2_0_Structure(const rapidjson::Document& doc) const {
    if (!doc.HasMember("steps") || !doc["steps"].IsArray()) {
        return false;
    }
    // v2.0.0 has simple steps array
    for (const auto& step : doc["steps"].GetArray()) {
        if (!step.IsObject()) return false;
        if (!step.HasMember("id") || !step.HasMember("plugin")) return false;
    }
    return true;
}

bool WorkflowVersionMigrator::ValidateV2_1_Structure(const rapidjson::Document& doc) const {
    if (!doc.HasMember("nodes") || !doc["nodes"].IsArray()) {
        return false;
    }
    if (!doc.HasMember("connections") || !doc["connections"].IsObject()) {
        return false;
    }
    // v2.1.0 nodes don't have typeVersion field
    return true;
}

bool WorkflowVersionMigrator::ValidateV2_2_Structure(const rapidjson::Document& doc) const {
    if (!doc.HasMember("nodes") || !doc["nodes"].IsArray()) {
        return false;
    }
    if (!doc.HasMember("connections") || !doc["connections"].IsObject()) {
        return false;
    }
    // v2.2.0 nodes must have type, typeVersion, and position
    for (const auto& node : doc["nodes"].GetArray()) {
        if (!node.IsObject()) return false;
        if (!node.HasMember("id") || !node.HasMember("type") ||
            !node.HasMember("typeVersion") || !node.HasMember("position")) {
            return false;
        }
    }
    return true;
}

rapidjson::Document WorkflowVersionMigrator::MigrateV2_0_to_V2_1(
    const rapidjson::Document& doc) const {
    if (logger_) {
        logger_->Trace("WorkflowVersionMigrator", "MigrateV2_0_to_V2_1", "Starting migration");
    }

    rapidjson::Document migrated;
    migrated.SetObject();
    auto& allocator = migrated.GetAllocator();

    // Copy top-level fields
    if (doc.HasMember("name")) {
        migrated.AddMember("name", rapidjson::Value(doc["name"]), allocator);
    }
    if (doc.HasMember("description")) {
        migrated.AddMember("description", rapidjson::Value(doc["description"]), allocator);
    }
    if (doc.HasMember("id")) {
        migrated.AddMember("id", rapidjson::Value(doc["id"]), allocator);
    }
    if (doc.HasMember("tenantId")) {
        migrated.AddMember("tenantId", rapidjson::Value(doc["tenantId"]), allocator);
    }

    // Convert steps to nodes
    if (doc.HasMember("steps") && doc["steps"].IsArray()) {
        rapidjson::Value nodes(rapidjson::kArrayType);
        int index = 0;

        for (const auto& step : doc["steps"].GetArray()) {
            rapidjson::Value node(rapidjson::kObjectType);
            node.AddMember("id", rapidjson::Value(step["id"]), allocator);
            node.AddMember("name", rapidjson::Value(step.HasMember("name") ? step["name"] : step["id"]), allocator);
            node.AddMember("type", rapidjson::Value(step["plugin"]), allocator);
            // v2.1.0: no typeVersion field
            node.AddMember("position", rapidjson::Value(rapidjson::kArrayType)
                .PushBack(index * 130, allocator)
                .PushBack(0, allocator), allocator);

            if (step.HasMember("inputs") && step["inputs"].IsObject()) {
                rapidjson::Value params(rapidjson::kObjectType);
                params.AddMember("inputs", rapidjson::Value(step["inputs"]), allocator);
                if (step.HasMember("outputs")) {
                    params.AddMember("outputs", rapidjson::Value(step["outputs"]), allocator);
                }
                node.AddMember("parameters", params, allocator);
            }

            nodes.PushBack(node, allocator);
            index++;
        }
        migrated.AddMember("nodes", nodes, allocator);
    }

    // Add empty connections for v2.1.0
    migrated.AddMember("connections", rapidjson::Value(rapidjson::kObjectType), allocator);

    // Set version
    migrated.AddMember("version", "2.1.0", allocator);

    return migrated;
}

rapidjson::Document WorkflowVersionMigrator::MigrateV2_1_to_V2_2(
    const rapidjson::Document& doc) const {
    if (logger_) {
        logger_->Trace("WorkflowVersionMigrator", "MigrateV2_1_to_V2_2", "Starting migration");
    }

    rapidjson::Document migrated;
    migrated.CopyFrom(doc, migrated.GetAllocator());
    auto& allocator = migrated.GetAllocator();

    // Add typeVersion to each node if missing
    if (migrated.HasMember("nodes") && migrated["nodes"].IsArray()) {
        for (auto& node : migrated["nodes"].GetArray()) {
            if (node.IsObject() && !node.HasMember("typeVersion")) {
                node.AddMember("typeVersion", 1, allocator);
            }
        }
    }

    // Add type field if missing (use old "plugin" field as fallback)
    if (migrated.HasMember("nodes") && migrated["nodes"].IsArray()) {
        for (auto& node : migrated["nodes"].GetArray()) {
            if (node.IsObject()) {
                if (!node.HasMember("type") && node.HasMember("plugin")) {
                    rapidjson::Value typeVal;
                    typeVal.CopyFrom(node["plugin"], allocator);
                    node.AddMember("type", typeVal, allocator);
                }
            }
        }
    }

    // Update version
    if (migrated.HasMember("version")) {
        migrated["version"].SetString("2.2.0", allocator);
    } else {
        migrated.AddMember("version", "2.2.0", allocator);
    }

    return migrated;
}

std::pair<rapidjson::Document, std::vector<std::string>> WorkflowVersionMigrator::Migrate(
    const rapidjson::Document& document,
    const std::string& targetVersion) const {
    if (logger_) {
        logger_->Trace("WorkflowVersionMigrator", "Migrate", "Entry", "Target: " + targetVersion);
    }

    std::string sourceVersion = DetectVersion(document);
    std::string normalized = NormalizeVersion(targetVersion);

    if (logger_) {
        logger_->Trace("WorkflowVersionMigrator", "Migrate",
            "Detected source version", sourceVersion);
    }

    if (!IsSupportedVersion(sourceVersion)) {
        throw std::runtime_error("Unsupported source version: " + sourceVersion);
    }

    if (!IsSupportedVersion(normalized)) {
        throw std::runtime_error("Unsupported target version: " + normalized);
    }

    std::vector<std::string> warnings;
    rapidjson::Document result = rapidjson::Document(document);

    // Migrate 1.0.0 -> reject
    if (NormalizeVersion(sourceVersion) == "1.0.0") {
        throw std::runtime_error(
            "Version 1.0.0 is no longer supported. Please update to v2.0.0 or later.");
    }

    // Migrate 2.0.0 -> 2.1.0
    if (NormalizeVersion(sourceVersion) == "2.0.0") {
        if (logger_) {
            logger_->Trace("WorkflowVersionMigrator", "Migrate",
                "Migrating from v2.0.0 to v2.1.0");
        }
        result = MigrateV2_0_to_V2_1(result);
        warnings.push_back("Migrated from v2.0.0 to v2.1.0: Converted steps array to nodes format");
        sourceVersion = "2.1.0";
    }

    // Migrate 2.1.0 -> 2.2.0
    if (NormalizeVersion(sourceVersion) == "2.1.0" && NormalizeVersion(normalized) == "2.2.0") {
        if (logger_) {
            logger_->Trace("WorkflowVersionMigrator", "Migrate",
                "Migrating from v2.1.0 to v2.2.0");
        }
        result = MigrateV2_1_to_V2_2(result);
        warnings.push_back("Migrated from v2.1.0 to v2.2.0: Added typeVersion and type fields");
    }

    return {result, warnings};
}

}  // namespace sdl3cpp::services::impl
