#include "workflow_runtime_config_step.hpp"

#include "../config/runtime_config_builder.hpp"
#include "workflow_step_io_resolver.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRuntimeConfigStep::WorkflowRuntimeConfigStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRuntimeConfigStep::GetPluginId() const {
    return "runtime.config.build";
}

void WorkflowRuntimeConfigStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string documentKey = resolver.GetRequiredInputKey(step, "document");
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "runtime");

    const auto* documentHandle = context.TryGet<std::shared_ptr<rapidjson::Document>>(documentKey);
    if (!documentHandle || !(*documentHandle)) {
        throw std::runtime_error("Workflow runtime.config.build missing document input '" + documentKey + "'");
    }

    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("Workflow runtime.config.build missing path input '" + pathKey + "'");
    }

    RuntimeConfigBuilder builder(logger_);
    RuntimeConfig config = builder.Build(**documentHandle, pathValue);
    context.Set(outputKey, std::move(config));
}

}  // namespace sdl3cpp::services::impl
