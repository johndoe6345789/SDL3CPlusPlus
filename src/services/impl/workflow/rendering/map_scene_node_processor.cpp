#include "services/interfaces/workflow/rendering/map_scene_node_processor.hpp"

#include "services/interfaces/workflow/rendering/map_mesh_geometry.hpp"
#include "services/interfaces/workflow/rendering/map_mesh_gpu_upload.hpp"
#include "services/interfaces/workflow/rendering/map_mesh_physics.hpp"

#include <string>

namespace sdl3cpp::services::impl {

void ProcessMapSceneNode(const aiScene* scene, const aiNode* node,
                         aiMatrix4x4 parentTransform, SDL_GPUDevice* device,
                         btDiscreteDynamicsWorld* world,
                         const MapLoadParams& params, WorkflowContext& context,
                         nlohmann::json& mapNodes, int& meshCount) {
    const aiMatrix4x4 transform = parentTransform * node->mTransformation;

    for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
        const aiMesh* mesh   = scene->mMeshes[node->mMeshes[m]];
        std::string meshName = node->mName.C_Str();
        if (meshName.empty()) {
            meshName = "map_mesh_" + std::to_string(meshCount);
        }

        ExtractedMapMesh extracted =
            ExtractMapMeshGeometry(mesh, transform, params.scale);
        if (extracted.vertices.empty()) continue;

        const MapMeshBuffers buffers =
            UploadMapMeshBuffers(device, extracted.vertices, extracted.indices);

        // Store with plane_ prefix (compatible with draw.textured).
        context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_vb", buffers.vb);
        context.Set<SDL_GPUBuffer*>("plane_" + meshName + "_ib", buffers.ib);
        context.Set("plane_" + meshName,
                    nlohmann::json{{"vertex_count", extracted.vertices.size()},
                                   {"index_count", extracted.indices.size()},
                                   {"stride", 20}});

        if (params.createPhysics) {
            auto* body = CreateMapMeshPhysicsBody(world, extracted.bbMin,
                                                  extracted.bbMax);
            if (body) {
                context.Set<btRigidBody*>("physics_body_" + meshName, body);
            }
        }

        // Track for the frame loop draw step.
        nlohmann::json nodeInfo = {
            {"name", meshName},
            {"index_count", extracted.indices.size()},
            {"bb_min",
             {extracted.bbMin.x, extracted.bbMin.y, extracted.bbMin.z}},
            {"bb_max",
             {extracted.bbMax.x, extracted.bbMax.y, extracted.bbMax.z}}};
        mapNodes.push_back(nodeInfo);
        meshCount++;
    }

    for (unsigned int c = 0; c < node->mNumChildren; ++c) {
        ProcessMapSceneNode(scene, node->mChildren[c], transform, device, world,
                            params, context, mapNodes, meshCount);
    }
}

}  // namespace sdl3cpp::services::impl
