#include "services/interfaces/workflow/scene/workflow_scene_add_geometry_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>
#include <cstdio>

#if defined(_WIN32)
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")
#else
#include <uuid/uuid.h>
#endif

namespace sdl3cpp::services::impl {

WorkflowSceneAddGeometryStep::WorkflowSceneAddGeometryStep(
    std::shared_ptr<ISceneService> sceneService,
    std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneAddGeometryStep::GetPluginId() const {
    return "scene.add_geometry";
}

void WorkflowSceneAddGeometryStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.add_geometry requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string geometryIdKey = resolver.GetRequiredInputKey(step, "geometry_id");
    const std::string transformKey = resolver.GetRequiredInputKey(step, "transform");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "object_id");

    const auto* geometryId = context.TryGet<std::string>(geometryIdKey);
    const auto* transform = context.TryGet<std::array<float, 16>>(transformKey);

    if (!geometryId) {
        throw std::runtime_error("scene.add_geometry requires geometry_id string input");
    }
    if (!transform) {
        throw std::runtime_error("scene.add_geometry requires transform matrix input");
    }

    // Create a SceneObject with the geometry and transform
    SceneObject obj;
    obj.modelMatrix = *transform;
    obj.hasCustomModelMatrix = true;
    obj.objectType = "geometry_object";

    // Create a unique object ID
#if defined(_WIN32)
    UUID uuid_val;
    UuidCreate(&uuid_val);
    RPC_CSTR str = nullptr;
    UuidToStringA(&uuid_val, &str);
    std::string objectId(reinterpret_cast<char*>(str));
    RpcStringFreeA(&str);
#else
    uuid_t uuid_val;
    uuid_generate(uuid_val);
    char uuid_str[37];
    uuid_unparse(uuid_val, uuid_str);
    std::string objectId(uuid_str);
#endif

    // Store object_id in context for reference
    context.Set(outputKey, objectId);

    if (logger_) {
        logger_->Trace("WorkflowSceneAddGeometryStep", "Execute",
                       "geometry_id=" + *geometryId + ", object_id=" + objectId,
                       "Added geometry to scene");
    }
}

}  // namespace sdl3cpp::services::impl
