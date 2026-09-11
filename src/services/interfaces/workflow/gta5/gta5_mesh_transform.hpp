#pragma once

#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"

namespace sdl3cpp::services::impl {

/// Turn a vehicle to face +z, the engine's forward.
///
/// GTA models face +y, which the axis conversion maps to -z, so a car
/// read as it stands drives backwards and steers on its rear axle. A half
/// turn about up is (x, y, z) -> (-x, y, -z); its determinant is +1, so
/// winding and normals need nothing more.
void TurnGta5MeshAround(Gta5MeshData& mesh);

/// A pack wheel resized to one vehicle, and mirrored for one side.
///
/// Pack wheels are modelled about the origin with the axle along x and
/// the rim face towards -x, which is outward on the right-hand side of a
/// car facing +z (x < 0); the left-hand pair is mirrored. Mirroring
/// negates x, which reverses the winding, so each triangle is flipped
/// back. The tyre is scaled radially to `radius` and across to `width`.
Gta5MeshData ShapeGta5Wheel(const Gta5MeshData& wheel, float radius,
                            float width, bool mirror);

}  // namespace sdl3cpp::services::impl
