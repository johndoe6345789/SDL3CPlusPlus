#pragma once

#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

namespace sdl3cpp::services::impl {

/// A shot leaving the barrel.
void SpawnGta5Muzzle(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& ahead);

/// The round's streak.
void SpawnGta5Tracer(Gta5Effects& effects, const glm::vec3& from,
                     const glm::vec3& to);

/// Where a round struck. `stuck` says what it struck and where on it,
/// so a mark on something that moves is carried along; the default
/// leaves the mark standing in the world.
void SpawnGta5Impact(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal,
                     const Gta5Stuck& stuck = Gta5Stuck{});

/// A rocket going off.
void SpawnGta5Explosion(Gta5Effects& effects, const glm::vec3& at,
                        float radius);
/// The mark left behind.
void SpawnGta5Scorch(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal, float radius, float seconds,
                     int cell = -1, const Gta5Stuck& stuck = Gta5Stuck{});

/// Carry them forward `dt` seconds and drop the spent ones. Marks stuck
/// to something are placed from wherever that has moved to.
void UpdateGta5Effects(Gta5Effects& effects, float dt);

/// Forget every mark stuck to `body`, before whatever it is stuck to is
/// deleted: a mark holds that pointer to follow it, so it must not
/// outlive the thing.
void DropGta5MarksOn(Gta5Effects& effects, const btCollisionObject* body);

}  // namespace sdl3cpp::services::impl
