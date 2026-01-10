#include "workflow_config_load_step.hpp"
#include "../config/json_config_document_loader.hpp"
#include "workflow_step_io_resolver.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowConfigLoadStep::WorkflowConfigLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowConfigLoadStep::GetPluginId() const {
    return "config.load";
}

void WorkflowConfigLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");
    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("Workflow config.load missing path input '" + pathKey + "'");
    }

    json_config::JsonConfigDocumentLoader loader(logger_);
    auto document = std::make_shared<rapidjson::Document>(loader.Load(pathValue));

    const std::string outputKey = resolver.GetRequiredOutputKey(step, "document");
    context.Set(outputKey, std::move(document));
}

}  // namespace sdl3cpp::services::impl
