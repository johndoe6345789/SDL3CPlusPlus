#pragma once

#include "services/interfaces/workflow/rendering/map_load_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

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
