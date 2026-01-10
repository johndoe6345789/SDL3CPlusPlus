#include "json_config_version_validator.hpp"
#include "json_config_schema_version.hpp"
#include "services/interfaces/i_logger.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl::json_config {

namespace {
constexpr const char* kSchemaVersionKey = "schema_version";
constexpr const char* kConfigVersionKey = "configVersion";
}

JsonConfigVersionValidator::JsonConfigVersionValidator(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::optional<int> JsonConfigVersionValidator::ReadVersionField(
    const rapidjson::Value& document,
    const char* fieldName,
    const std::filesystem::path& configPath) const {
    if (!document.HasMember(fieldName)) {
        return std::nullopt;
    }
    const auto& value = document[fieldName];
    if (value.IsInt()) {
        return value.GetInt();
    }
    if (value.IsUint()) {
        return static_cast<int>(value.GetUint());
    }
    throw std::runtime_error("JSON member '" + std::string(fieldName) + "' must be an integer in " +
                             configPath.string());
}

std::optional<int> JsonConfigVersionValidator::Validate(const rapidjson::Value& document,
                                                        const std::filesystem::path& configPath) const {
    const auto schemaVersion = ReadVersionField(document, kSchemaVersionKey, configPath);
    const auto configVersion = ReadVersionField(document, kConfigVersionKey, configPath);
    if (schemaVersion && configVersion && *schemaVersion != *configVersion) {
        throw std::runtime_error("JSON members 'schema_version' and 'configVersion' must match in " +
                                 configPath.string());
    }
    const auto activeVersion = schemaVersion ? schemaVersion : configVersion;
    if (!activeVersion) {
        if (logger_) {
            logger_->Warn("JsonConfigService::LoadFromJson: Missing schema version in " +
                          configPath.string() + "; assuming version " +
                          std::to_string(kRuntimeConfigSchemaVersion));
        }
        return std::nullopt;
    }
    if (logger_) {
        logger_->Trace("JsonConfigService", "ValidateSchemaVersion",
                       "version=" + std::to_string(*activeVersion) +
                       ", configPath=" + configPath.string());
    }
    return activeVersion;
}

}  // namespace sdl3cpp::services::impl::json_config
