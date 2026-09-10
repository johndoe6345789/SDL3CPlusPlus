#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief How far above its origin an item sits this frame.
 *
 * cg_ents.c CG_Item: an item rides 4 units above its origin and bobs 4
 * either side of that, each one slightly out of phase with the next so a
 * row of them does not pulse in unison. Returned in engine units.
 */
float Q3ItemBobHeight(float timeSeconds, int index);

/// cg.autoAngles / cg.autoAnglesFast: a full turn every 2048ms, or every
/// 1024ms when @p fast (health, in Quake's item list).
float Q3ItemSpinYaw(float timeSeconds, bool fast);

/// Places a Z-up model at @p pos turned to @p yaw, the same remap the
/// world-placed MD3s use: model X is forward, Y right, Z up.
glm::mat4 Q3ItemMatrix(const glm::vec3& pos, float yaw);

}  // namespace sdl3cpp::services::impl
