#include "services/interfaces/workflow/scene/workflow_scene_create_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>
#include <cstdio>

#if defined(_WIN32)
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")
#else
#include <uuid/uuid.h>
#endif

namespace {
std::string generate_uuid() {
#if defined(_WIN32)
    UUID uuid_val;
    UuidCreate(&uuid_val);
    RPC_CSTR str = nullptr;
    UuidToStringA(&uuid_val, &str);
    std::string result(reinterpret_cast<char*>(str));
    RpcStringFreeA(&str);
    return result;
#else
    uuid_t uuid_val;
    uuid_generate(uuid_val);
    char uuid_str[37];
    uuid_unparse(uuid_val, uuid_str);
    return std::string(uuid_str);
#endif
}
}  // namespace

namespace sdl3cpp::services::impl {

WorkflowSceneCreateStep::WorkflowSceneCreateStep(std::shared_ptr<ISceneService> sceneService,
                                                 std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneCreateStep::GetPluginId() const {
    return "scene.create";
}

void WorkflowSceneCreateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.create requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "scene_id");

    std::string sceneId = generate_uuid();

    // Store scene_id in context
    context.Set(outputKey, sceneId);

    if (logger_) {
        logger_->Trace("WorkflowSceneCreateStep", "Execute",
                       "scene_id=" + sceneId,
                       "Created new scene");
    }
}

}  // namespace sdl3cpp::services::impl
