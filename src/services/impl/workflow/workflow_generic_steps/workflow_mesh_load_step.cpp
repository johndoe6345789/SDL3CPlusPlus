#include "workflow_mesh_load_step.hpp"

#include "../workflow_step_io_resolver.hpp"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowMeshLoadStep::WorkflowMeshLoadStep(std::shared_ptr<IMeshService> meshService,
                                           std::shared_ptr<ILogger> logger)
    : meshService_(std::move(meshService)),
      logger_(std::move(logger)) {}

std::string WorkflowMeshLoadStep::GetPluginId() const {
    return "mesh.load";
}

void WorkflowMeshLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!meshService_) {
        throw std::runtime_error("mesh.load requires an IMeshService");
    }

    WorkflowStepIoResolver resolver;
    const std::string pathKey = resolver.GetRequiredInputKey(step, "path");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "mesh");

    std::filesystem::path pathValue;
    if (const auto* path = context.TryGet<std::filesystem::path>(pathKey)) {
        pathValue = *path;
    } else if (const auto* pathString = context.TryGet<std::string>(pathKey)) {
        pathValue = *pathString;
    } else {
        throw std::runtime_error("mesh.load missing path input '" + pathKey + "'");
    }

    MeshPayload payload;
    std::string error;
    if (!meshService_->LoadFromFile(pathValue.string(), payload, error)) {
        throw std::runtime_error("mesh.load failed: " + error);
    }

    context.Set(outputKey, std::move(payload));

    if (logger_) {
        logger_->Trace("WorkflowMeshLoadStep", "Execute",
                       "path=" + pathValue.string() +
                           ", output=" + outputKey,
                       "Loaded mesh payload");
    }
}

}  // namespace sdl3cpp::services::impl
