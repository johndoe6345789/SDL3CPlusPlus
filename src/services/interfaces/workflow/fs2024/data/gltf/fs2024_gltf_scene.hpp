#pragma once

#include <glm/mat4x4.hpp>
#include <nlohmann/json_fwd.hpp>
#include <vector>

namespace sdl3cpp::fs2024 {

/// The accumulated world transform of every node in `gltf`, indexed
/// the same as its own `nodes` array. FS2024's own landmark models
/// place most meshes on child nodes with their own
/// translation/rotation/scale rather than baking it into the mesh
/// data itself, so this must be applied before a primitive's
/// positions mean anything in the model's own space.
std::vector<glm::mat4> ComputeNodeWorldTransforms(const nlohmann::json& gltf);

}  // namespace sdl3cpp::fs2024
