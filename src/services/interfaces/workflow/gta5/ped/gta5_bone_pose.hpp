#pragma once

#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Each bone's rest pose in the drawable's own (GTA) space: its local
/// rotation, translation and scale composed up the parent chain. The
/// skeleton is at drawable +0x18: bones (0x50 each) at +0x20, their
/// count at +0x5E; a bone keeps a quaternion (x, y, z, w) at +0x00,
/// translation at +0x10, scale at +0x20 and its parent (i16) at +0x32.
/// Empty when there is no skeleton.
std::vector<glm::mat4> ReadGta5BonePose(const Gta5Resource& res,
                                        std::int64_t drawable);

/// The pose a model's geometry hangs off, or null. The model's binding
/// (u32 at +0x28) names its bone in the top byte and whether it is
/// skinned in the second; skinned geometry is left alone -- a car's
/// body is -- and so is a bone the skeleton does not have.
const glm::mat4* Gta5ModelPose(const Gta5Resource& res, std::int64_t model,
                               const std::vector<glm::mat4>& pose);

/// Move a part's engine-space vertices and normals by a bone pose given
/// in GTA space. A traffic light's arm is modelled at its bone, and
/// drawn without this it lay across the road at the pole's foot.
void PoseGta5Part(Gta5SubMeshData& part, const glm::mat4& pose);

}  // namespace sdl3cpp::services::impl
