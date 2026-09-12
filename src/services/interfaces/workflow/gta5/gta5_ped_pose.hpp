#pragma once

#include "services/interfaces/workflow/gta5/gta5_skeleton.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// A turn of `degrees` about `axis`, in the ped own space: it looks
/// along +z, +x is its right and +y is up.
glm::mat3 Gta5About(float degrees,
                    glm::vec3 axis = glm::vec3(1.f, 0.f, 0.f));

/// Turn bone `name` by `turn`, given in the ped space at rest, about
/// its own joint: in the bone frame it stays a hinge as parents move.
void Gta5Turn(const Gta5Skeleton& s, std::vector<glm::mat4>& locals,
              const char* name, const glm::mat3& turn);

/// The bind pose holds the arms out in a T. This swings one from there
/// to point along `want`, in the ped space.
glm::mat3 Gta5ArmTowards(const Gta5Skeleton& s, const char* arm,
                         const char* fore, const glm::vec3& want);

/// Where an arm hangs with nothing in it: down, and a little away from
/// the body on its own side.
glm::vec3 Gta5ArmRest(const Gta5Skeleton& s, const char* arm,
                      const char* fore);

}  // namespace sdl3cpp::services::impl
