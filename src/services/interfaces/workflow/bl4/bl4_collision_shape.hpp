#pragma once

#include "services/interfaces/workflow/bl4/bl4_geometry.hpp"
#include "services/interfaces/workflow/bl4/bl4_mesh_extract.hpp"

namespace sdl3cpp::services::impl {

/// Builds the shared, unit-scale collision mesh for an archetype from
/// the same mesh that was uploaded for drawing, so what you see is what
/// you walk on -- q3.pm.*'s TraceBox() sweeps whatever's in the physics
/// world, so this is the entire bl4/Quake-3-kinematics integration.
///
/// Returns false for a mesh with no triangles. The arrays it fills live
/// on the geometry because Bullet indexes them rather than copying.
bool BuildBl4CollisionShape(const Bl4MeshData& mesh, Bl4Geometry& geometry);

/// Frees the collision shape and its backing arrays.
void ReleaseBl4CollisionShape(Bl4Geometry& geometry);

}  // namespace sdl3cpp::services::impl
