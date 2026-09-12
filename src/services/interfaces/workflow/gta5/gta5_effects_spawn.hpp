#pragma once

#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

namespace sdl3cpp::services::impl {

/// A shot leaving the barrel.
void SpawnGta5Muzzle(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& ahead);

/// The round's streak.
void SpawnGta5Tracer(Gta5Effects& effects, const glm::vec3& from,
                     const glm::vec3& to);

/// Where a round struck.
void SpawnGta5Impact(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal);

/// A rocket going off.
void SpawnGta5Explosion(Gta5Effects& effects, const glm::vec3& at,
                        float radius);
/// The mark left behind.
void SpawnGta5Scorch(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal, float radius, float seconds,
                     int cell = -1);

/// Carry them forward `dt` seconds and drop the spent ones.
void UpdateGta5Effects(Gta5Effects& effects, float dt);

}  // namespace sdl3cpp::services::impl
