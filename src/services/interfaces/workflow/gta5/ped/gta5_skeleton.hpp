#pragma once

#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A drawable's skeleton, bones in file order.
struct Gta5Skeleton {
    std::vector<std::string> names;
    std::vector<int> parents;       // -1 for a root
    std::vector<glm::mat4> locals;  // rest pose, relative to the parent
    std::vector<glm::mat4> rest;    // rest pose, in the drawable's space
    std::vector<glm::mat4> unbind;  // inverse of rest: bind pose to bone
};

/// Read the skeleton at drawable +0x18 -- ReadGta5BonePose has the
/// layout; a bone's name pointer is at +0x38. False when there is none.
bool ReadGta5Skeleton(const Gta5Resource& res, std::int64_t drawable,
                      Gta5Skeleton& out);

/// The bone named `name`, or -1.
int FindGta5Bone(const Gta5Skeleton& skeleton, const std::string& name);

/// `locals` composed down the hierarchy: each bone's parent's pose times
/// its own. A parent always comes before its children.
void ComposeGta5Pose(const Gta5Skeleton& skeleton,
                     const std::vector<glm::mat4>& locals,
                     std::vector<glm::mat4>& out);

}  // namespace sdl3cpp::services::impl
