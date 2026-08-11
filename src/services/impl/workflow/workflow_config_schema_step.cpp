#include "services/interfaces/workflow/workflow_config_schema_step.hpp"
#include "services/interfaces/config/json_config_schema_validator.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowConfigSchemaStep::WorkflowConfigSchemaStep(std::shared_ptr<ILogger> logger,
                                                   std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {
    if (logger_) {
        logger_->Trace("WorkflowConfigSchemaStep", "Constructor", "Entry");
    }
}

std::string WorkflowConfigSchemaStep::GetPluginId() const {
    if (logger_) {
        logger_->Trace("WorkflowConfigSchemaStep", "GetPluginId", "Entry");
    }
    return "config.schema.validate";
}

void WorkflowConfigSchemaStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowConfigSchemaStep", "Execute", "Entry");
    }
    WorkflowStepIoResolver resolver;
    const std::string documentKey = resolver.GetRequiredInputKey(step, "document");
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");
    const auto* document = context.TryGet<std::shared_ptr<rapidjson::Document>>(documentKey);
    if (!document || !(*document)) {
        throw std::runtime_error("Workflow config.schema.validate missing document input '" + documentKey + "'");
    }

    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("Workflow config.schema.validate missing path input '" + pathKey + "'");
    }

    json_config::JsonConfigSchemaValidator validator(logger_, probeService_);
    validator.ValidateOrThrow(**document, pathValue);
}

}  // namespace sdl3cpp::services::impl
