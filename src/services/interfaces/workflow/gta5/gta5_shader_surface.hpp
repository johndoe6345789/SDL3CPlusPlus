#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// What one shader of a drawable asks for.
///
/// The texture cannot say any of the flags. A leaf billboard's shape
/// lives in its alpha; a decal -- tyre tracks, dirt, road paint -- is
/// blended over what is under it; a painted panel's texture is a few
/// white pixels coloured per vehicle from carcols.ymt. The shader says
/// all three: its render bucket (+0x39), checked against the mask at
/// +0x3C, (1 << bucket) | 0xFF00, is 3 for cutouts and 1 (alpha) or 2
/// (decal) for blended surfaces; and its name hash (+0x00) names the
/// vehicle_paint family. Terrain is known by its layered textures.
struct Gta5ShaderSurface {
    std::uint32_t texture{0};  // diffuse, by name hash; 0 for none
    bool paint{false};
    bool cutout{false};
    bool blend{false};
    /// Four layers weighted by the vertex colour; see gta5_terrain.frag.
    bool terrain{false};
    std::array<std::uint32_t, 4> layers{};
};

/// One entry per shader of the drawable at `drawable`: ShaderGroup at
/// +0x10, its shader list at +0x10 of that.
std::vector<Gta5ShaderSurface> ReadGta5ShaderSurfaces(
    const Gta5Resource& res, std::int64_t drawable);

}  // namespace sdl3cpp::services::impl
