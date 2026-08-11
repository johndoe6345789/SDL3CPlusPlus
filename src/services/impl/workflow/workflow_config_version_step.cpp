#include "services/interfaces/workflow/workflow_config_version_step.hpp"
#include "services/interfaces/config/json_config_version_validator.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowConfigVersionStep::WorkflowConfigVersionStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowConfigVersionStep", "Constructor", "Entry");
    }
}

std::string WorkflowConfigVersionStep::GetPluginId() const {
    if (logger_) {
        logger_->Trace("WorkflowConfigVersionStep", "GetPluginId", "Entry");
    }
    return "config.version.validate";
}

void WorkflowConfigVersionStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowConfigVersionStep", "Execute", "Entry");
    }
    WorkflowStepIoResolver resolver;
    const std::string documentKey = resolver.GetRequiredInputKey(step, "document");
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");
    const auto* document = context.TryGet<std::shared_ptr<rapidjson::Document>>(documentKey);
    if (!document || !(*document)) {
        throw std::runtime_error("Workflow config.version.validate missing document input '" + documentKey + "'");
    }

    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("Workflow config.version.validate missing path input '" + pathKey + "'");
    }

    json_config::JsonConfigVersionValidator validator(logger_);
    const auto version = validator.Validate(**document, pathValue);

    const std::string outputKey = resolver.GetRequiredOutputKey(step, "version");
    context.Set(outputKey, version);
}

}  // namespace sdl3cpp::services::impl
