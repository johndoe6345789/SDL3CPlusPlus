#include "services/interfaces/workflow/scene/workflow_scene_get_bounds_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

using json = nlohmann::json;

WorkflowSceneGetBoundsStep::WorkflowSceneGetBoundsStep(std::shared_ptr<ISceneService> sceneService,
                                                       std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneGetBoundsStep::GetPluginId() const {
    return "scene.get_bounds";
}

void WorkflowSceneGetBoundsStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.get_bounds requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "bounds");

    // Get combined vertices from scene
    const auto& vertices = sceneService_->GetCombinedVertices();

    // Calculate bounding box
    json boundsJson;
    if (vertices.empty()) {
        boundsJson["min"] = {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}};
        boundsJson["max"] = {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}};
    } else {
        float minX = vertices[0].position[0], maxX = vertices[0].position[0];
        float minY = vertices[0].position[1], maxY = vertices[0].position[1];
        float minZ = vertices[0].position[2], maxZ = vertices[0].position[2];

        for (const auto& v : vertices) {
            if (v.position[0] < minX) minX = v.position[0];
            if (v.position[0] > maxX) maxX = v.position[0];
            if (v.position[1] < minY) minY = v.position[1];
            if (v.position[1] > maxY) maxY = v.position[1];
            if (v.position[2] < minZ) minZ = v.position[2];
            if (v.position[2] > maxZ) maxZ = v.position[2];
        }

        boundsJson["min"] = {{"x", minX}, {"y", minY}, {"z", minZ}};
        boundsJson["max"] = {{"x", maxX}, {"y", maxY}, {"z", maxZ}};
    }

    // Store bounds as JSON string in context
    context.Set(outputKey, boundsJson.dump());

    if (logger_) {
        logger_->Trace("WorkflowSceneGetBoundsStep", "Execute",
                       "bounds=" + boundsJson.dump(),
                       "Got scene bounding box");
    }
}

}  // namespace sdl3cpp::services::impl
