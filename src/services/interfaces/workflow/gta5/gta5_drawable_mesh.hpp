#pragma once

#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// The high-detail geometries of the drawable at `drawable`, one part
/// per geometry, in engine space.
///
/// Drawable +0x50 is the high-detail model list; each model lists its
/// geometries at +0x08 and their shader indices (u16) at +0x20. A
/// geometry keeps its vertex buffer at +0x18, index buffer at +0x38 and
/// index count at +0x58, with the index data at +0x18 of its buffer.
/// `paint` tints vehicle_paint geometry; the map passes white.
Gta5MeshData ReadGta5DrawableMesh(const Gta5Resource& res,
                                  std::int64_t drawable,
                                  const glm::vec3& paint = glm::vec3(1.f));

/// Offset of an archetype's drawable inside a loaded resource: 0 in a
/// .ydr, the fragment's drawable (+0x30) in a .yft, and the matching
/// entry of a .ydd, which holds name hashes at +0x20 and drawables at
/// +0x30. -1 when it is not there.
std::int64_t LocateGta5Drawable(const Gta5Resource& res,
                                const std::string& path,
                                std::uint32_t hash);

}  // namespace sdl3cpp::services::impl
