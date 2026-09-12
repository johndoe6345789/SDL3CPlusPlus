#pragma once

#include "services/interfaces/workflow/gta5/effects/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"

namespace sdl3cpp::services::impl {

/// Where a shot really met a car, in the car's own space.
///
/// Bullet tests a single box drawn round the whole car, so the point it
/// reports stands off the bodywork -- over a bonnet it is up at roof
/// height, and a mark placed there hangs in the air. The drawn mesh is
/// what a bullet should mark, so this walks the car's own triangles for
/// the nearest one the shot crosses. False when the shot passed through
/// the box without meeting the body, and the caller should keep what
/// Bullet gave it.
bool Gta5StickToCar(const Gta5Vehicle& car, const glm::vec3& from,
                    const glm::vec3& to, Gta5Stuck& stuck);

/// `stuck` back in the world, as it stands now.
glm::vec3 Gta5StuckAt(const Gta5Stuck& stuck);

}  // namespace sdl3cpp::services::impl
