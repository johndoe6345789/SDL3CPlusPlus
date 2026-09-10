#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Vertex format used by map.load's uploaded meshes: float3 pos + float2 uv
/// (20 bytes), matching draw.textured's expected layout.
struct PosUvVertex {
    float x, y, z, u, v;
};

/// map.load's resolved parameters: the model file to import, a uniform
/// scale applied to every vertex, and whether to build a static physics
/// body per mesh from its bounding box.
struct MapLoadParams {
    std::string filePath;
    float scale        = 1.0f;
    bool createPhysics  = true;
};

/// Reads `file_path`/`scale`/`create_physics`, falling back from a step
/// parameter to a wired context input (by `step.inputs`) for `file_path`,
/// exactly as map.load always has.
MapLoadParams ReadMapLoadParams(const WorkflowStepDefinition& step,
                                const WorkflowContext& context);

/// One mesh's world-space vertices/indices plus its bounding box, ready to
/// upload to the GPU and/or turn into a static physics body.
struct ExtractedMapMesh {
    std::vector<PosUvVertex> vertices;
    std::vector<uint16_t> indices;
    glm::vec3 bbMin{1e9f, 1e9f, 1e9f};
    glm::vec3 bbMax{-1e9f, -1e9f, -1e9f};
};

/// Applies `transform` and `scale` to every vertex of `mesh`, tracking the
/// resulting bounding box; UVs default to (0,0) when the mesh has none.
ExtractedMapMesh ExtractMapMeshGeometry(const aiMesh* mesh,
                                        const aiMatrix4x4& transform,
                                        float scale);

/// The GPU vertex/index buffers created for one uploaded map mesh.
struct MapMeshBuffers {
    SDL_GPUBuffer* vb = nullptr;
    SDL_GPUBuffer* ib = nullptr;
};

/// Creates and uploads `vertices`/`indices` via one transfer buffer and one
/// copy pass, exactly as map.load always has.
MapMeshBuffers UploadMapMeshBuffers(SDL_GPUDevice* device,
                                    const std::vector<PosUvVertex>& vertices,
                                    const std::vector<uint16_t>& indices);

/// Builds a static (mass 0) box body sized to [bbMin, bbMax] and adds it to
/// `world`; returns null when the box would be degenerate (any axis
/// smaller than 0.01) or `world` is null.
btRigidBody* CreateMapMeshPhysicsBody(btDiscreteDynamicsWorld* world,
                                      const glm::vec3& bbMin,
                                      const glm::vec3& bbMax);

/**
 * @brief Recursively imports one Assimp scene node's meshes and children.
 *
 * For each mesh: extracts geometry, uploads GPU buffers under the
 * `plane_<meshName>` keys draw.textured/draw.map expect, optionally builds
 * a physics body, and appends a `map.nodes` entry. Then recurses into the
 * node's children with the accumulated transform, exactly as map.load's
 * original in-place lambda did.
 */
void ProcessMapSceneNode(const aiScene* scene, const aiNode* node,
                         aiMatrix4x4 parentTransform, SDL_GPUDevice* device,
                         btDiscreteDynamicsWorld* world,
                         const MapLoadParams& params, WorkflowContext& context,
                         nlohmann::json& mapNodes, int& meshCount);

}  // namespace sdl3cpp::services::impl
