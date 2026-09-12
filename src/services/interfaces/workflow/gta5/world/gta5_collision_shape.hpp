#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"

namespace sdl3cpp::services::impl {

/// Build the shared, unit-scale collision mesh for an archetype from the
/// same mesh that was uploaded for drawing, so what you see is what you
/// walk on.
///
/// Returns false for a mesh with no triangles. The arrays it fills live
/// on the geometry because Bullet indexes them rather than copying.
bool BuildGta5CollisionShape(const Gta5MeshData& mesh,
                             Gta5Geometry& geometry);

/// Free the collision shape and its backing arrays.
void ReleaseGta5CollisionShape(Gta5Geometry& geometry);

}  // namespace sdl3cpp::services::impl
