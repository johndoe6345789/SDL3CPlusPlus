#include "json_config_schema_validator.hpp"
#include "json_config_document_parser.hpp"
#include "json_config_schema_path_resolver.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"

#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl::json_config {

namespace {
std::string PointerToString(const rapidjson::Pointer& pointer) {
    rapidjson::StringBuffer buffer;
    pointer.Stringify(buffer);
    return buffer.GetString();
}
}

JsonConfigSchemaValidator::JsonConfigSchemaValidator(std::shared_ptr<ILogger> logger,
                                                     std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

void JsonConfigSchemaValidator::ValidateOrThrow(const rapidjson::Document& document,
                                                const std::filesystem::path& configPath) const {
    JsonConfigSchemaPathResolver resolver;
    const auto schemaPath = resolver.Resolve(configPath);
    if (schemaPath.empty()) {
        if (logger_) {
            logger_->Warn("JsonConfigService::ValidateSchemaDocument: Schema file not found for " +
                          configPath.string());
        }
        return;
    }

    JsonConfigDocumentParser parser;
    rapidjson::Document schemaDocument = parser.Parse(schemaPath, "schema file");
    rapidjson::SchemaDocument schema(schemaDocument);
    rapidjson::SchemaValidator validator(schema);
    if (!document.Accept(validator)) {
        const std::string docPointer = PointerToString(validator.GetInvalidDocumentPointer());
        const std::string schemaPointer = PointerToString(validator.GetInvalidSchemaPointer());
        const std::string keyword = validator.GetInvalidSchemaKeyword();
        const std::string message = "JSON schema validation failed at " + docPointer +
            " (schema " + schemaPointer + ", keyword=" + keyword + ")";
        if (logger_) {
            logger_->Error("JsonConfigService::ValidateSchemaDocument: " + message +
                           " configPath=" + configPath.string());
        }
        if (probeService_) {
            ProbeReport report{};
            report.severity = ProbeSeverity::Error;
            report.code = "CONFIG_SCHEMA_INVALID";
            report.jsonPath = docPointer;
            report.message = message;
            report.details = "schemaPointer=" + schemaPointer + ", keyword=" + keyword;
            probeService_->Report(report);
        }
        throw std::runtime_error("JSON schema validation failed for " + configPath.string() +
                                 " at " + docPointer + " (schema " + schemaPointer +
                                 ", keyword=" + keyword + ")");
    }
    if (logger_) {
        logger_->Trace("JsonConfigService", "ValidateSchemaDocument",
                       "schemaPath=" + schemaPath.string() +
                       ", configPath=" + configPath.string(),
                       "Schema validation passed");
    }
}

}  // namespace sdl3cpp::services::impl::json_config
