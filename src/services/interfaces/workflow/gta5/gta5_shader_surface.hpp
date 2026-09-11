#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// What one shader of a drawable asks for.
///
/// The texture cannot say either of the two flags. A leaf billboard's
/// shape lives in its alpha, and a painted panel's texture is a few
/// white pixels coloured per vehicle from carcols.ymt. The shader says
/// both: its render bucket (+0x39) is 3 for cutouts, checked against the
/// mask at +0x3C, (1 << bucket) | 0xFF00; and its name hash (+0x00)
/// names the vehicle_paint family.
struct Gta5ShaderSurface {
    std::uint32_t texture{0};  // diffuse, by name hash; 0 for none
    bool paint{false};
    bool cutout{false};
};

/// One entry per shader of the drawable at `drawable`: ShaderGroup at
/// +0x10, its shader list at +0x10 of that.
std::vector<Gta5ShaderSurface> ReadGta5ShaderSurfaces(
    const Gta5Resource& res, std::int64_t drawable);

}  // namespace sdl3cpp::services::impl
