#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::q3 {

/**
 * @brief Which way is up, and how Quake's axes map onto the engine's.
 *
 * Quake is Z-up and right-handed: +X forward, +Y left, +Z up. An engine
 * that wants the same numbers ioq3 prints — and the same formulae copied
 * out of bg_pmove.c without a mental transpose — should stay in that
 * convention, which is what `Ioq3ZUp` is.
 *
 * `EngineYUp` is the alternative for a renderer or asset pipeline that
 * expects +Y up, mapping a Quake point (x, y, z) to (x, z, -y). The
 * permutation has determinant +1, so it is a rotation: handedness,
 * cross products and winding order all survive it.
 */
enum class AxisConvention {
    Ioq3ZUp,
    EngineYUp,
};

/// The convention the engine core is built in. Quake content, Quake
/// formulae and Quake units all assume Ioq3ZUp; change this only for a
/// use case that genuinely needs another basis, and expect to convert at
/// the boundary rather than halfway through.
AxisConvention ActiveAxisConvention();

/// Overrides the active convention. Intended for a host application to
/// call once at start-up, and for tests.
void SetActiveAxisConvention(AxisConvention convention);

/// World up in the active convention.
glm::vec3 AxisUp(AxisConvention convention);
inline glm::vec3 AxisUp() {
    return AxisUp(ActiveAxisConvention());
}

/// A Quake-space point (Z-up, Quake units) in the active convention,
/// scaled by @p scale. Identity but for the scale under Ioq3ZUp.
glm::vec3 FromQuakePoint(const glm::vec3& quake, float scale,
                         AxisConvention convention);
inline glm::vec3 FromQuakePoint(const glm::vec3& quake, float scale) {
    return FromQuakePoint(quake, scale, ActiveAxisConvention());
}

/// A Quake-space direction in the active convention. Same permutation as
/// FromQuakePoint without the scale, so normals stay unit length.
glm::vec3 FromQuakeDir(const glm::vec3& quake, AxisConvention convention);
inline glm::vec3 FromQuakeDir(const glm::vec3& quake) {
    return FromQuakeDir(quake, ActiveAxisConvention());
}

/// The horizontal direction @p yaw radians round the up axis, measured
/// the way ioq3's AngleVectors measures it: yaw 0 faces Quake +X.
glm::vec3 YawForward(float yaw, AxisConvention convention);
inline glm::vec3 YawForward(float yaw) {
    return YawForward(yaw, ActiveAxisConvention());
}

/// The yaw that faces @p delta, the inverse of YawForward. The vertical
/// component is ignored.
float YawTowards(const glm::vec3& delta, AxisConvention convention);
inline float YawTowards(const glm::vec3& delta) {
    return YawTowards(delta, ActiveAxisConvention());
}

/**
 * @brief The rotation that faces a model along @p forward with @p up.
 *
 * Which column each axis lands on depends on the convention, because the
 * MD3 loader permutes vertices as it decodes them: under Ioq3ZUp they
 * stay Quake's (X forward, Y left, Z up) and this is ioq3's
 * AnglesToAxis; under EngineYUp they have already become (x, z, -y), so
 * the model's local +Y is up and its local +Z is right. Building this
 * basis by hand at each draw site is what laid every model on its side.
 */
glm::mat4 ModelBasis(const glm::vec3& forward, const glm::vec3& up,
                     AxisConvention convention);
inline glm::mat4 ModelBasis(const glm::vec3& forward, const glm::vec3& up) {
    return ModelBasis(forward, up, ActiveAxisConvention());
}

/// Places a model at @p pos, turned @p yaw about the up axis.
glm::mat4 PlaceModel(const glm::vec3& pos, float yaw,
                     AxisConvention convention);
inline glm::mat4 PlaceModel(const glm::vec3& pos, float yaw) {
    return PlaceModel(pos, yaw, ActiveAxisConvention());
}

/// Places a model at @p pos along an explicit basis, for a viewmodel
/// that follows the camera rather than a yaw.
glm::mat4 PlaceModelWithBasis(const glm::vec3& pos, const glm::vec3& forward,
                              const glm::vec3& up, AxisConvention convention);
inline glm::mat4 PlaceModelWithBasis(const glm::vec3& pos,
                                     const glm::vec3& forward,
                                     const glm::vec3& up) {
    return PlaceModelWithBasis(pos, forward, up, ActiveAxisConvention());
}

}  // namespace sdl3cpp::q3
