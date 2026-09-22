#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief An original cockpit dashboard: a wheel rim and two dials.
 *
 * Built directly in normalised device coordinates (x,y in [-1,1], z
 * fixed near the camera) so it draws as a screen-space overlay with
 * an identity MVP, on top of whatever the 3D scene already drew.
 * Reuses the textured pipeline and the palette texture (see
 * stunts_shape_mesh.hpp) purely for solid colour, one draw call, no
 * new shader.
 *
 * This is an original layout inspired by the genre's usual driver's
 * view -- a wheel below the windscreen flanked by a speedometer and
 * a tachometer -- not a reproduction of any specific game's artwork.
 *
 * @param speedFrac  Current speed as a 0..1 fraction of the gauge's
 *                   own range (`speedMax`).
 * @param rpmFrac    Current RPM as a 0..1 fraction of the redline.
 */
GeometryPlaneMesh BuildStuntsDashboardMesh(float speedFrac, float rpmFrac);

}  // namespace sdl3cpp::services::impl
