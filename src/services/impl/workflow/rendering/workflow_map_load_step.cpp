#include "services/interfaces/workflow/rendering/workflow_map_load_step.hpp"
#include "services/interfaces/workflow/rendering/map_load_params.hpp"
#include "services/interfaces/workflow/rendering/map_scene_node_processor.hpp"

#include <SDL3/SDL_gpu.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowMapLoadStep::WorkflowMapLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowMapLoadStep::GetPluginId() const {
    return "map.load";
}

void WorkflowMapLoadStep::Execute(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    const MapLoadParams params = ReadMapLoadParams(step, context);
    if (params.filePath.empty()) {
        throw std::runtime_error("map.load: 'file_path' parameter is required");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("map.load: GPU device not found");

    // Load scene with Assimp.
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        params.filePath, aiProcess_Triangulate | aiProcess_GenNormals |
                             aiProcess_FlipUVs |
                             aiProcess_JoinIdenticalVertices);
    if (!scene || !scene->mRootNode) {
        throw std::runtime_error("map.load: Failed to load '" +
                                 params.filePath +
                                 "': " + importer.GetErrorString());
    }

    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    nlohmann::json mapNodes = nlohmann::json::array();
    int meshCount           = 0;
    aiMatrix4x4 identity;
    ProcessMapSceneNode(scene, scene->mRootNode, identity, device, world,
                        params, context, mapNodes, meshCount);

    context.Set("map.nodes", mapNodes);

    if (logger_) {
        logger_->Info("map.load: Loaded '" + params.filePath + "' (" +
                      std::to_string(meshCount) + " meshes, " +
                      std::to_string(scene->mNumMeshes) + " total in file)");
    }
}

}  // namespace sdl3cpp::services::impl
